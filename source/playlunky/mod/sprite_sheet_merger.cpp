#include "sprite_sheet_merger.h"

#include "dds_conversion.h"
#include "entity_data_extraction.h"
#include "extract_game_assets.h"
#include "log.h"
#include "playlunky_settings.h"
#include "util/algorithms.h"
#include "util/format.h"
#include "util/on_scope_exit.h"
#include "virtual_filesystem.h"

#include <spel2.h>

#include <cassert>
#include <zip_adaptor.h>

#pragma warning(push)
#pragma warning(disable : 5054)
#pragma warning(disable : 4996)
#pragma warning(disable : 4244)
#include <opencv2/imgproc.hpp>
#pragma warning(pop)

SpriteSheetMerger::SpriteSheetMerger(const PlaylunkySettings& settings)
    : mRandomCharacterSelectEnabled{ settings.GetBool("settings", "random_character_select", false) || settings.GetBool("sprite_settings", "random_character_select", false) }, mGenerateCharacterJournalStickersEnabled{ settings.GetBool("sprite_settings", "generate_character_journal_stickers", true) }, mGenerateCharacterJournalEntriesEnabled{ settings.GetBool("sprite_settings", "generate_character_journal_entries", true) }, mGenerateStickerPixelArtEnabled{ settings.GetBool("sprite_settings", "generate_sticker_pixel_art", true) }
{
}
SpriteSheetMerger::~SpriteSheetMerger() = default;

void SpriteSheetMerger::GatherSheetData(bool force_regen_char_journal, bool force_regen_char_stickers)
{
    m_EntityDataExtractor = std::make_unique<EntityDataExtractor>();
    m_EntityDataExtractor->PreloadEntityMappings();

    MakeItemsSheet();
    MakeJournalItemsSheet();
    MakeJournalMonstersSheet();
    MakeJournalMonstersBigSheet();
    MakeJournalPeopleSheet(force_regen_char_journal);
    MakeJournalStickerSheet(force_regen_char_stickers);
    MakeMountsTargetSheet();
    MakePetsTargetSheet();
    MakeMonstersTargetSheet();
    MakeBigMonstersTargetSheet();
    MakeCharacterTargetSheet("black");
    MakeCharacterTargetSheet("blue");
    MakeCharacterTargetSheet("cerulean");
    MakeCharacterTargetSheet("cinnabar");
    MakeCharacterTargetSheet("cyan");
    MakeCharacterTargetSheet("eggchild");
    MakeCharacterTargetSheet("gold");
    MakeCharacterTargetSheet("gray");
    MakeCharacterTargetSheet("green");
    MakeCharacterTargetSheet("hired");
    MakeCharacterTargetSheet("iris");
    MakeCharacterTargetSheet("khaki");
    MakeCharacterTargetSheet("lemon");
    MakeCharacterTargetSheet("lime");
    MakeCharacterTargetSheet("magenta");
    MakeCharacterTargetSheet("olive");
    MakeCharacterTargetSheet("orange");
    MakeCharacterTargetSheet("pink");
    MakeCharacterTargetSheet("red");
    MakeCharacterTargetSheet("violet");
    MakeCharacterTargetSheet("white");
    MakeCharacterTargetSheet("yellow");
    MakeMenuLeaderTargetSheet();
    MakeMenuBasicTargetSheet();
    MakeCaveDecoTargetSheet();

    m_EntityDataExtractor = nullptr;
}

void SpriteSheetMerger::RegisterSheet(const std::filesystem::path& full_sheet, bool outdated, bool deleted)
{
    auto full_sheet_no_ext = std::filesystem::path{ full_sheet }.replace_extension();
    if (RegisteredSourceSheet* registered_sheet = algo::find_if(m_RegisteredSourceSheets,
                                                                [&full_sheet_no_ext](const RegisteredSourceSheet& sheet)
                                                                { return algo::is_same_path(sheet.Path, full_sheet_no_ext); }))
    {
        registered_sheet->Outdated = registered_sheet->Outdated || outdated;
        registered_sheet->Deleted = registered_sheet->Deleted || deleted;
        return;
    }
    m_RegisteredSourceSheets.push_back(RegisteredSourceSheet{
        .Path = std::move(full_sheet_no_ext),
        .Outdated = outdated,
        .Deleted = deleted });
}
void SpriteSheetMerger::RegisterCustomImages(const std::filesystem::path& base_path, const std::filesystem::path& original_data_folder, std::int64_t priority, const CustomImages& custom_images)
{
    namespace fs = std::filesystem;

    auto get_image = [this](const fs::path& image_path) -> Image&
    {
        if (auto* image = algo::find_if(m_CachedImages,
                                        [&image_path](const LoadedImage& image)
                                        { return algo::is_same_path(image.ImagePath, image_path); }))
        {
            return *image->ImageFile;
        }
        m_CachedImages.push_back({ image_path, std::make_unique<Image>() });
        m_CachedImages.back().ImageFile->Load(image_path);
        return *m_CachedImages.back().ImageFile;
    };

    for (const auto& [relative_path, custom_image] : custom_images)
    {
        const auto absolute_path = base_path / relative_path;
        if (!fs::exists(absolute_path))
        {
            LogError("Custom image mapping from file {} is registered for mod {}, but the file does not exist in the mod...", relative_path, base_path.filename().string());
            continue;
        }

        const Image& source_image = get_image(absolute_path);

        for (const auto& [target_sheet, custom_image_map] : custom_image.ImageMap)
        {
            const auto target_sheet_no_ext = fs::path{ target_sheet }.replace_extension();
            if (TargetSheet* existing_target_sheet = algo::find(m_TargetSheets, &TargetSheet::Path, target_sheet_no_ext))
            {
                auto it = std::upper_bound(existing_target_sheet->SourceSheets.begin(), existing_target_sheet->SourceSheets.end(), priority, [](std::int64_t prio, const SourceSheet& sheet)
                                           { return sheet.Priority < prio; });
                existing_target_sheet->SourceSheets.insert(
                    it,
                    SourceSheet{
                        .Path{ relative_path },
                        .RootPath{ base_path },
                        .Size{ .Width{ source_image.GetWidth() }, .Height{ source_image.GetHeight() } },
                        .TileMap{ custom_image_map } });
                existing_target_sheet->ForceRegen = existing_target_sheet->ForceRegen || custom_image.Outdated;
            }
            else
            {
                const auto target_sheet_dds = fs::path{ target_sheet }.replace_extension(".DDS");
                if (ExtractGameAssets(std::array{ target_sheet_dds }, original_data_folder))
                {
                    const auto target_file_path = original_data_folder / fs::path{ target_sheet_no_ext }.replace_extension(".png");
                    const Image& target_image = get_image(target_file_path);

                    const auto source_sheets = std::vector<SourceSheet>{
                        SourceSheet{
                            .Path{ relative_path },
                            .RootPath{ base_path },
                            .Priority{ priority },
                            .Size{ .Width{ source_image.GetWidth() }, .Height{ source_image.GetHeight() } },
                            .TileMap{ custom_image_map } }
                    };

                    m_TargetSheets.push_back(TargetSheet{
                        .Path{ target_sheet_no_ext },
                        .Size{ .Width{ target_image.GetWidth() }, .Height{ target_image.GetHeight() } },
                        .SourceSheets{ std::move(source_sheets) },
                        .ForceRegen{ custom_image.Outdated } });
                }
                else
                {
                    LogError("Failed extracting game asset {} required by mod {}...", target_sheet, base_path.filename().string());
                }
            }
        }
    }
}

bool SpriteSheetMerger::NeedsRegeneration(const std::filesystem::path& destination_folder) const
{
    for (const TargetSheet& target_sheet : m_TargetSheets)
    {
        if (NeedsRegen(target_sheet, destination_folder))
        {
            return true;
        }
    }
    return false;
}

bool SpriteSheetMerger::NeedsRegen(const TargetSheet& target_sheet, const std::filesystem::path& destination_folder) const
{
    namespace fs = std::filesystem;

    if (target_sheet.ForceRegen)
    {
        return true;
    }

    const bool does_exist = fs::exists(fs::path{ destination_folder / target_sheet.Path }.replace_extension(".DDS"));
    const bool random_select = target_sheet.RandomSelect;
    for (const SourceSheet& source_sheet : target_sheet.SourceSheets)
    {
        const auto source_path_no_ext = source_sheet.Path.has_extension()
                                            ? fs::path{ source_sheet.Path }.replace_extension()
                                            : source_sheet.Path;
        if (const RegisteredSourceSheet* registered_sheet = algo::find_if(m_RegisteredSourceSheets,
                                                                          [&source_path_no_ext](const RegisteredSourceSheet& sheet)
                                                                          { return algo::is_same_path(sheet.Path, source_path_no_ext); }))
        {
            if (!does_exist || random_select || registered_sheet->Outdated || registered_sheet->Deleted)
            {
                return true;
            }
        }
    }
    for (const MultiSourceTile& multi_source_sheets : target_sheet.MultiSourceTiles)
    {
        for (const auto& path : multi_source_sheets.Paths)
        {
            if (const RegisteredSourceSheet* registered_sheet = algo::find_if(m_RegisteredSourceSheets,
                                                                              [&path](const RegisteredSourceSheet& sheet)
                                                                              { return algo::is_same_path(sheet.Path, path); }))
            {
                if (!does_exist || random_select || registered_sheet->Outdated || registered_sheet->Deleted)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool SpriteSheetMerger::GenerateRequiredSheets(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder, VirtualFilesystem& vfs, bool force_reload)
{
    OnScopeExit clear_sheet_data{
        [this]()
        {
            for (RegisteredSourceSheet& sheet : m_RegisteredSourceSheets)
            {
                sheet.Outdated = false;
                sheet.Deleted = false;
            }
            m_CachedImages.clear();
        },
    };

    namespace fs = std::filesystem;

    auto get_image = [this](const fs::path& image_path) -> Image&
    {
        if (auto* image = algo::find(m_CachedImages, &LoadedImage::ImagePath, image_path))
        {
            return *image->ImageFile;
        }
        m_CachedImages.push_back({ image_path, std::make_unique<Image>() });
        m_CachedImages.back().ImageFile->Load(image_path);
        return *m_CachedImages.back().ImageFile;
    };

    std::array allowed_extensions{
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
    for (const TargetSheet& target_sheet : m_TargetSheets)
    {
        if (NeedsRegen(target_sheet, destination_folder))
        {
            // TODO: Don't get .DDS!
            const auto target_file_path = vfs.GetFilePathFilterExt(target_sheet.Path, allowed_extensions).value_or(fs::path{ source_folder / target_sheet.Path }.replace_extension(".png"));
            Image target_image = get_image(target_file_path).Clone();

            static auto validate_source_aspect_ratio = [](const SourceSheet& source_sheet, const Image& source_image)
            {
                // Skip images with wrong aspect ratio
                const std::uint64_t aspect_ratio_offset =
                    std::abs(
                        static_cast<int64_t>(source_sheet.Size.Width) * source_image.GetHeight() - static_cast<int64_t>(source_image.GetWidth()) * source_sheet.Size.Height);
                // We accept images that are 10 pixels off in width
                static constexpr std::uint64_t s_AcceptedPixelError{ 10 };
                return aspect_ratio_offset <= source_sheet.Size.Height * s_AcceptedPixelError;
            };

            float upscaling = 1.0f;

            std::vector<std::optional<fs::path>> target_sheet_paths;
            for (const SourceSheet& source_sheet : target_sheet.SourceSheets)
            {
                auto source_file_path = [&, random_select = target_sheet.RandomSelect]() -> std::optional<fs::path>
                {
                    if (source_sheet.RootPath)
                    {
                        return source_sheet.RootPath.value() / source_sheet.Path;
                    }
                    else if (!random_select)
                    {
                        return vfs.GetFilePathFilterExt(source_sheet.Path, allowed_extensions);
                    }
                    else
                    {
                        return vfs.GetRandomFilePathFilterExt(source_sheet.Path, allowed_extensions);
                    }
                }();

                if (source_file_path)
                {
                    const Image& source_image = get_image(source_file_path.value());

                    if (!validate_source_aspect_ratio(source_sheet, source_image))
                    {
                        source_file_path.reset();
                    }
                    else
                    {
                        const float source_width_scaling = static_cast<float>(source_image.GetWidth()) / source_sheet.Size.Width;
                        const float source_height_scaling = static_cast<float>(source_image.GetHeight()) / source_sheet.Size.Height;
                        upscaling = std::max(std::max(upscaling, source_width_scaling), source_height_scaling);
                    }
                }

                target_sheet_paths.push_back(std::move(source_file_path));
            }

            const float original_target_width_scaling = static_cast<float>(target_image.GetWidth()) / target_sheet.Size.Width;
            const float original_target_height_scaling = static_cast<float>(target_image.GetHeight()) / target_sheet.Size.Height;
            const float adjusted_upscaling = upscaling / std::min(original_target_width_scaling, original_target_height_scaling);

            target_image.Resize(ImageSize{
                .x{ static_cast<std::uint32_t>(adjusted_upscaling * target_sheet.Size.Width) },
                .y{ static_cast<std::uint32_t>(adjusted_upscaling * target_sheet.Size.Height) } });

            const float target_width_scaling = static_cast<float>(target_image.GetWidth()) / target_sheet.Size.Width;
            const float target_height_scaling = static_cast<float>(target_image.GetHeight()) / target_sheet.Size.Height;

            for (const auto [source_sheet, source_file_path] : zip::zip(target_sheet.SourceSheets, target_sheet_paths))
            {
                if (source_file_path)
                {
                    const Image& source_image = get_image(source_file_path.value());

                    const float source_width_scaling = static_cast<float>(source_image.GetWidth()) / source_sheet.Size.Width;
                    const float source_height_scaling = static_cast<float>(source_image.GetHeight()) / source_sheet.Size.Height;

                    for (const TileMapping& tile_mapping : source_sheet.TileMap)
                    {
                        const ImageSubRegion source_region = ImageSubRegion{
                            .x{ static_cast<std::int32_t>(tile_mapping.SourceTile.Left * source_width_scaling) },
                            .y{ static_cast<std::int32_t>(tile_mapping.SourceTile.Top * source_height_scaling) },
                            .width{ static_cast<std::uint32_t>((tile_mapping.SourceTile.Right - tile_mapping.SourceTile.Left) * source_width_scaling) },
                            .height{ static_cast<std::uint32_t>((tile_mapping.SourceTile.Bottom - tile_mapping.SourceTile.Top) * source_height_scaling) },
                        };
                        const ImageSubRegion target_region = ImageSubRegion{
                            .x{ static_cast<std::int32_t>(tile_mapping.TargetTile.Left * target_height_scaling) },
                            .y{ static_cast<std::int32_t>(tile_mapping.TargetTile.Top * target_height_scaling) },
                            .width{ static_cast<std::uint32_t>((tile_mapping.TargetTile.Right - tile_mapping.TargetTile.Left) * target_height_scaling) },
                            .height{ static_cast<std::uint32_t>((tile_mapping.TargetTile.Bottom - tile_mapping.TargetTile.Top) * target_height_scaling) },
                        };

                        if (!source_image.ContainsSubRegion(source_region))
                        {
                            LogError("Source image {} does not contain tile ({}, {}, {}, {}), image size is ({}, {})... Tile expected from target image {}...", source_file_path.value().string(), source_region.x, source_region.y, source_region.width, source_region.height, source_image.GetWidth(), source_image.GetHeight(), target_file_path.string());
                            continue;
                        }
                        if (!target_image.ContainsSubRegion(target_region))
                        {
                            LogError("Target image {} does not contain tile ({}, {}, {}, {}), image size is ({}, {})... Tile expected from source image {}...", target_file_path.string(), target_region.x, target_region.y, target_region.width, target_region.height, target_image.GetWidth(), target_image.GetHeight(), source_file_path.value().string());
                            continue;
                        }

                        Image source_tile = source_image.CloneSubImage(source_region);
                        const auto target_size = ::ImageSize{ .x{ static_cast<std::uint32_t>(target_region.width) }, .y{ static_cast<std::uint32_t>(target_region.height) } };

                        if (source_sheet.Processing)
                        {
                            source_tile = source_sheet.Processing(std::move(source_tile), target_size);
                        }

                        if (source_tile.GetWidth() != target_size.x || source_tile.GetHeight() != target_size.y)
                        {
                            source_tile.Resize(target_size);
                        }

                        try
                        {
                            target_image.Blit(source_tile, target_region);
                        }
                        catch (cv::Exception& e)
                        {
                            fmt::print("{}", e.what());
                        }
                    }
                }
            }

            for (const MultiSourceTile& multi_source_sheet : target_sheet.MultiSourceTiles)
            {
                std::vector<std::pair<Image, std::filesystem::path>> tiles;
                auto front_tile = multi_source_sheet.TileMap.front();
                const ImageSubRegion target_region = ImageSubRegion{
                    .x{ static_cast<std::int32_t>(front_tile.TargetTile.Left * target_height_scaling) },
                    .y{ static_cast<std::int32_t>(front_tile.TargetTile.Top * target_height_scaling) },
                    .width{ static_cast<std::uint32_t>((front_tile.TargetTile.Right - front_tile.TargetTile.Left) * target_height_scaling) },
                    .height{ static_cast<std::uint32_t>((front_tile.TargetTile.Bottom - front_tile.TargetTile.Top) * target_height_scaling) },
                };
                const auto target_size = ::ImageSize{ .x{ static_cast<std::uint32_t>(target_region.width) }, .y{ static_cast<std::uint32_t>(target_region.height) } };

                for (auto [path, size, tile_mapping] : zip::zip(multi_source_sheet.Paths, multi_source_sheet.Sizes, multi_source_sheet.TileMap))
                {
                    auto source_file_path = [&, random_select = target_sheet.RandomSelect]() -> std::optional<fs::path>
                    {
                        if (!random_select)
                        {
                            return vfs.GetFilePathFilterExt(path, allowed_extensions);
                        }
                        else
                        {
                            return vfs.GetRandomFilePathFilterExt(path, allowed_extensions);
                        }
                    }();

                    if (source_file_path)
                    {
                        const Image& source_image = get_image(source_file_path.value());

                        const float source_width_scaling = static_cast<float>(source_image.GetWidth()) / size.Width;
                        const float source_height_scaling = static_cast<float>(source_image.GetHeight()) / size.Height;

                        const ImageSubRegion source_region = ImageSubRegion{
                            .x{ static_cast<std::int32_t>(tile_mapping.SourceTile.Left * source_width_scaling) },
                            .y{ static_cast<std::int32_t>(tile_mapping.SourceTile.Top * source_height_scaling) },
                            .width{ static_cast<std::uint32_t>((tile_mapping.SourceTile.Right - tile_mapping.SourceTile.Left) * source_width_scaling) },
                            .height{ static_cast<std::uint32_t>((tile_mapping.SourceTile.Bottom - tile_mapping.SourceTile.Top) * source_height_scaling) },
                        };

                        if (!source_image.ContainsSubRegion(source_region))
                        {
                            LogError("Source image {} does not contain tile ({}, {}, {}, {}), image size is ({}, {})... Tile expected from target image {}...", source_file_path.value().string(), source_region.x, source_region.y, source_region.width, source_region.height, source_image.GetWidth(), source_image.GetHeight(), target_file_path.string());
                            continue;
                        }

                        Image source_tile = source_image.CloneSubImage(source_region);
                        tiles.push_back({ source_tile.Clone(), std::move(source_file_path).value() });
                    }
                }

                if (!target_image.ContainsSubRegion(target_region))
                {
                    LogError("Target image {} does not contain tile ({}, {}, {}, {}), image size is ({}, {}) needed for multi-source target...", target_file_path.string(), target_region.x, target_region.y, target_region.width, target_region.height, target_image.GetWidth(), target_image.GetHeight());
                    continue;
                }

                Image source_tile = multi_source_sheet.Processing(std::move(tiles), target_size);
                if (source_tile.GetWidth() != target_size.x || source_tile.GetHeight() != target_size.y)
                {
                    source_tile.Resize(target_size);
                }

                try
                {
                    target_image.Blit(source_tile, target_region);
                }
                catch (cv::Exception& e)
                {
                    fmt::print("{}", e.what());
                }
            }

            const auto destination_file_path = fs::path{ destination_folder / target_sheet.Path }.replace_extension(".DDS");
            if (!ConvertRBGAToDds(target_image.GetData(), target_image.GetWidth(), target_image.GetHeight(), destination_file_path))
            {
                return false;
            }
            else if (force_reload)
            {
                Spelunky_ReloadTexture(fs::path{ target_sheet.Path }.replace_extension(".DDS").string().c_str());
            }
        }
    }

    return true;
}
