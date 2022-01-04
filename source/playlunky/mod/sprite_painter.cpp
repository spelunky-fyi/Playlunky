#include "sprite_painter.h"

#include "dds_conversion.h"
#include "image_processing.h"
#include "known_files.h"
#include "log.h"
#include "playlunky_settings.h"
#include "sprite_sheet_merger.h"
#include "util/algorithms.h"
#include "util/on_scope_exit.h"
#include "virtual_filesystem.h"

#include <spel2.h>

inline constexpr std::uint32_t c_ReloadDelay = 250;

SpritePainter::SpritePainter(SpriteSheetMerger& merger, VirtualFilesystem& vfs, const PlaylunkySettings& /*settings*/)
    : m_Merger{ merger }
    , m_Vfs{ vfs }
{
}
SpritePainter::~SpritePainter() = default;

void SpritePainter::RegisterSheet(std::filesystem::path full_path, std::filesystem::path db_destination, bool outdated, bool deleted)
{
    if (deleted)
    {
        const auto [real_path, real_db_destination] = ConvertToRealFilePair(full_path, db_destination);
        if (algo::contains(s_KnownTextureFiles, std::filesystem::path{ real_path }.replace_extension("").filename().string()))
        {
            const auto dds_db_destination = std::filesystem::path{ real_db_destination }.replace_extension(".DDS");
            std::filesystem::remove(dds_db_destination);
        }
        else
        {
            std::filesystem::remove(real_db_destination);
        }
    }
    else
    {
        m_RegisteredColorModSheets.push_back({ std::move(full_path), std::move(db_destination), outdated });
    }
}

void SpritePainter::FinalizeSetup(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder)
{
    for (const auto& sheet : m_RegisteredColorModSheets)
    {
        //if (sheet.outdated || true)
        {
            RepaintImage(sheet.full_path, sheet.db_destination);
        }
    }
    m_Merger.GenerateRequiredSheets(source_folder, destination_folder, m_Vfs, true);
}

void SpritePainter::Update(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder)
{
    if (m_HasPendingReloads)
    {
        const std::size_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (now - m_ReloadTimestamp > c_ReloadDelay)
        {
            algo::erase_if(m_PendingRepaints, [this](auto& pending_reload)
                           {
                               return RepaintImage(pending_reload.sheet->full_path, pending_reload.sheet->db_destination);
                           });
            if (m_HasPendingReloads && m_PendingRepaints.empty())
            {
                m_Merger.GenerateRequiredSheets(source_folder, destination_folder, m_Vfs, true);
                m_HasPendingReloads = false;
            }
        }
    }
}

bool SpritePainter::RepaintImage(const std::filesystem::path& full_path, const std::filesystem::path& db_destination)
{
    LogInfo("Repainting image {}...", full_path.string());

    const auto [real_path, real_db_destination] = ConvertToRealFilePair(full_path, db_destination);

    if (const auto source_path = GetSourcePath(real_path))
    {
        Image repainted_image;
        repainted_image.Load(source_path.value());

        Image color_mod_image;
        color_mod_image.Load(full_path);
        repainted_image = ColorBlend(std::move(color_mod_image), std::move(repainted_image));

        const auto luminance_image_path = ReplaceColExtension(full_path, "_lumin");
        if (std::filesystem::exists(luminance_image_path))
        {
            Image luminance_mod_image;
            luminance_mod_image.Load(luminance_image_path);
            repainted_image = LuminanceBlend(std::move(luminance_mod_image), std::move(repainted_image));
        }

        if (algo::contains(s_KnownTextureFiles, std::filesystem::path{ real_path }.replace_extension("").filename().string()))
        {
            // Save to .DDS
            const auto dds_db_destination = std::filesystem::path{ real_db_destination }.replace_extension(".DDS");
            ConvertRBGAToDds(repainted_image.GetData(), repainted_image.GetWidth(), repainted_image.GetHeight(), dds_db_destination);

            // Make game reload
            std::string dds_path = std::filesystem::path{ real_path }.replace_extension(".DDS").string();
            std::replace(dds_path.begin(), dds_path.end(), '\\', '/');
            Spelunky_ReloadTexture(dds_path.c_str());
        }
        else
        {
            // Save to .png (or possibly other source format, should work too)
            repainted_image.Write(real_db_destination);

            m_Merger.RegisterSheet(real_path, true, false);
        }

        LogInfo("File {} was successfully repainted...", full_path.string());
    }

    return true;
}
SpritePainter::FilePair SpritePainter::ConvertToRealFilePair(const std::filesystem::path& full_path, const std::filesystem::path& db_destination)
{
    auto rel_path = [&]()
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

            std::filesystem::path rel_path{ strip_mod_dir(full_path) };
            return ReplaceColExtension(std::move(rel_path));
        }
        else
        {
            return ReplaceColExtension(full_path);
        }
    }();

    return { rel_path, ReplaceColExtension(db_destination) };
}
std::optional<std::filesystem::path> SpritePainter::GetSourcePath(const std::filesystem::path& relative_path)
{
    static const std::array allowed_extensions{
        std::filesystem::path{ ".png" },
        std::filesystem::path{ ".bmp" },
        std::filesystem::path{ ".jpg" },
        std::filesystem::path{ ".jpeg" },
        std::filesystem::path{ ".jpe" },
        std::filesystem::path{ ".jp2" },
        std::filesystem::path{ ".tif" },
        std::filesystem::path{ ".tiff" },
        std::filesystem::path{ ".pbm" },
        std::filesystem::path{ ".pgm" },
        std::filesystem::path{ ".ppm" },
        std::filesystem::path{ ".sr" },
        std::filesystem::path{ ".ras" },
    };
    return m_Vfs.GetFilePathFilterExt(relative_path, allowed_extensions, VfsType::User);
}
std::filesystem::path SpritePainter::ReplaceColExtension(std::filesystem::path path, std::string_view replacement)
{
    const auto old_file_name = path.filename();
    const auto old_stem = path.stem().string();
    const auto new_file_name = old_stem.substr(0, old_stem.size() - 4) + std::string{ replacement } + path.extension().string();
    return path.replace_filename(new_file_name);
}
