#include "sprite_hot_loader.h"

#include "dds_conversion.h"
#include "log.h"
#include "playlunky_settings.h"
#include "sprite_sheet_merger.h"
#include "util/algorithms.h"
#include "util/on_scope_exit.h"

#pragma warning(push)
#pragma warning(disable : 4927)
#include <FileWatch.hpp>
#pragma warning(pop)

SpriteHotLoader::SpriteHotLoader(SpriteSheetMerger& merger, const PlaylunkySettings& settings)
    : m_Merger{ merger }
    , m_ReloadDelay{ static_cast<std::uint32_t>(settings.GetInt("sprite_settings", "sprite_hot_load_delay", 15)) }
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
            if (!algo::contains(m_PendingReloads, &sheet))
            {
                m_PendingReloads.push_back(&sheet);
                m_HasPendingReloads = true;
            }
            m_ReloadTimer = m_ReloadDelay;
        };
        m_FileWatchers.push_back(std::make_unique<filewatch::FileWatch<std::filesystem::path>>(sheet.full_path, hot_load_sprite));
    }
}

void SpriteHotLoader::Update(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder, VirtualFilesystem& vfs)
{
    std::lock_guard lock{ m_PendingReloadsMutex };
    if ((m_ReloadTimer > 0 && --m_ReloadTimer == 0) || (m_ReloadTimer == 0 && m_HasPendingReloads))
    {
        algo::erase_if(m_PendingReloads, [this](const auto* sheet)
                       { return PrepareHotLoad(sheet->full_path, sheet->db_destination); });
        if (m_HasPendingReloads && m_PendingReloads.empty())
        {
            m_Merger.GenerateRequiredSheets(source_folder, destination_folder, vfs, true);
            m_HasPendingReloads = false;
        }
    }
}

bool SpriteHotLoader::PrepareHotLoad(const std::filesystem::path& full_path, const std::filesystem::path& db_destination)
{
    if (ConvertImageToDds(full_path, db_destination))
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
    return true;
}
