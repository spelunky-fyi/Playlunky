#include "sprite_hot_loader.h"

#include "dds_conversion.h"
#include "known_files.h"
#include "log.h"
#include "playlunky_settings.h"
#include "sprite_sheet_merger.h"
#include "util/algorithms.h"
#include "util/on_scope_exit.h"

#include <spel2.h>

#include <chrono>

#pragma warning(push)
#pragma warning(disable : 4927)
#include <FileWatch.hpp>
#pragma warning(pop)

SpriteHotLoader::SpriteHotLoader(SpriteSheetMerger& merger, const PlaylunkySettings& settings)
    : m_Merger{ merger }
    , m_ReloadDelay{ static_cast<std::uint32_t>(settings.GetInt("sprite_settings", "sprite_hot_load_delay", 500)) }
{
}
SpriteHotLoader::~SpriteHotLoader() = default;

void SpriteHotLoader::RegisterSheet(std::filesystem::path full_path, std::filesystem::path db_destination)
{
    m_RegisteredSheets.push_back({ std::move(full_path), std::move(db_destination) });
}

void SpriteHotLoader::FinalizeSetup()
{
    for (const auto& sheet : m_RegisteredSheets)
    {
        auto hot_load_sprite = [&, this](const std::filesystem::path&, const filewatch::Event change_type)
        {
            // Register index, do all in Update
            if (change_type == filewatch::Event::removed || change_type == filewatch::Event::renamed_old)
            {
                return;
            }

            std::lock_guard lock{ m_PendingReloadsMutex };
            if (!algo::contains(m_PendingReloads, &PendingReload::sheet, &sheet))
            {
                m_PendingReloads.push_back(PendingReload{ &sheet });
                m_HasPendingReloads = true;
            }
            m_ReloadTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        };
        m_FileWatchers.push_back(std::make_unique<filewatch::FileWatch<std::filesystem::path>>(sheet.full_path, hot_load_sprite));
    }
}

void SpriteHotLoader::Update(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder, VirtualFilesystem& vfs)
{
    std::lock_guard lock{ m_PendingReloadsMutex };
    if (m_HasPendingReloads)
    {
        const std::size_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (now - m_ReloadTimestamp > m_ReloadDelay)
        {
            algo::erase_if(m_PendingReloads, [this](auto& pending_reload)
                           {
                               pending_reload.has_warned = !PrepareHotLoad(pending_reload.sheet->full_path, pending_reload.sheet->db_destination, !pending_reload.has_warned);
                               return !pending_reload.has_warned; });
            if (m_HasPendingReloads && m_PendingReloads.empty())
            {
                m_Merger.GenerateRequiredSheets(source_folder, destination_folder, vfs, true);
                m_HasPendingReloads = false;
            }
        }
    }
}

bool SpriteHotLoader::PrepareHotLoad(const std::filesystem::path& full_path, const std::filesystem::path& db_destination, bool emit_info)
{
    if (emit_info)
    {
        LogInfo("Detected change in file {}, trying to reload it...", full_path.string());
    }

    // Try to load image, might not be written correctly yet  :/
    {
        Image test_image{};
        test_image.Load(full_path);
        if (test_image.IsEmpty())
        {
            if (emit_info)
            {
                LogInfo("File is not full written yet, trying periodically to reload...");
            }

            const std::size_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            m_ReloadTimestamp = now - 100;
            return false;
        }
    }

    if (!ConvertImageToDds(full_path, db_destination))
    {
        return false;
    }

    const std::filesystem::path path = [&]()
    {
        if (*full_path.begin() == "Mods")
        {
            auto strip_mod_dir = [](const std::filesystem::path& p) -> std::filesystem::path
            {
                auto strip_mod_dir_impl = [](const std::filesystem::path& p, auto& self) -> std::filesystem::path
                {
                    const std::filesystem::path& parent_path = p.parent_path();
                    if (parent_path.empty() || parent_path.string() == "Mods/Packs")
                    {
                        return std::filesystem::path();
                    }
                    else
                    {
                        return self(parent_path, self) / p.filename();
                    }
                };

                return strip_mod_dir_impl(p, strip_mod_dir_impl);
            };

            const std::filesystem::path rel_path{ strip_mod_dir(full_path) };
            return rel_path;
        }
        else
        {
            return full_path;
        }
    }();

    m_Merger.RegisterSheet(path, true, false);

    if (algo::contains(s_KnownTextureFiles, std::filesystem::path{ path }.replace_extension("").filename().string()))
    {
        std::string dds_path = std::filesystem::path{ path }.replace_extension(".DDS").string();
        std::replace(dds_path.begin(), dds_path.end(), '\\', '/');
        Spelunky_ReloadTexture(dds_path.c_str());
    }

    LogInfo("File {} was successfully reloaded...", full_path.string());

    return true;
}
