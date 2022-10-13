#include "dm_preview_merger.h"

#include "level_data.h"
#include "level_parser.h"
#include "util/algorithms.h"
#include "util/format.h"
#include "virtual_filesystem.h"

#include <zip_adaptor.h>

DmPreviewMerger::DmPreviewMerger(const PlaylunkySettings& /*settings*/)
{
}
DmPreviewMerger::~DmPreviewMerger() = default;

void DmPreviewMerger::RegisterDmLevel(std::filesystem::path path, bool outdated, bool deleted)
{
    if (RegisteredDmLevel* registered_level = algo::find_if(mDmLevels,
                                                            [&path](const RegisteredDmLevel& sheet)
                                                            { return algo::is_same_path(sheet.Path, path); }))
    {
        registered_level->Outdated = registered_level->Outdated || outdated;
        registered_level->Deleted = registered_level->Deleted || deleted;
        return;
    }
    mDmLevels.push_back(RegisteredDmLevel{
        .Path = std::move(path),
        .Outdated = outdated,
        .Deleted = deleted });
}

bool DmPreviewMerger::NeedsRegeneration(const std::filesystem::path& destination_folder) const
{
    namespace fs = std::filesystem;
    const bool does_exist = fs::exists(fs::path{ destination_folder / "dmpreview.tok" });
    static auto requires_update = [](const RegisteredDmLevel& registered_level)
    {
        return registered_level.Outdated || registered_level.Deleted;
    };
    return !does_exist || algo::contains_if(mDmLevels, requires_update);
}

bool DmPreviewMerger::GenerateDmPreview(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder, VirtualFilesystem& vfs)
{
    namespace fs = std::filesystem;
    using namespace std::string_view_literals;

    static constexpr std::string_view dmpreview_tok_path{ "Data/Levels/Arena/dmpreview.tok" };
    static constexpr std::array arena_levels{
        "Data/Levels/Arena/dm1-1.lvl"sv,
        "Data/Levels/Arena/dm1-2.lvl"sv,
        "Data/Levels/Arena/dm1-3.lvl"sv,
        "Data/Levels/Arena/dm1-4.lvl"sv,
        "Data/Levels/Arena/dm1-5.lvl"sv,
        "Data/Levels/Arena/dm2-1.lvl"sv,
        "Data/Levels/Arena/dm2-2.lvl"sv,
        "Data/Levels/Arena/dm2-3.lvl"sv,
        "Data/Levels/Arena/dm2-4.lvl"sv,
        "Data/Levels/Arena/dm2-5.lvl"sv,
        "Data/Levels/Arena/dm3-1.lvl"sv,
        "Data/Levels/Arena/dm3-2.lvl"sv,
        "Data/Levels/Arena/dm3-3.lvl"sv,
        "Data/Levels/Arena/dm3-4.lvl"sv,
        "Data/Levels/Arena/dm3-5.lvl"sv,
        "Data/Levels/Arena/dm4-1.lvl"sv,
        "Data/Levels/Arena/dm4-2.lvl"sv,
        "Data/Levels/Arena/dm4-3.lvl"sv,
        "Data/Levels/Arena/dm4-4.lvl"sv,
        "Data/Levels/Arena/dm4-5.lvl"sv,
        "Data/Levels/Arena/dm5-1.lvl"sv,
        "Data/Levels/Arena/dm5-2.lvl"sv,
        "Data/Levels/Arena/dm5-3.lvl"sv,
        "Data/Levels/Arena/dm5-4.lvl"sv,
        "Data/Levels/Arena/dm5-5.lvl"sv,
        "Data/Levels/Arena/dm6-1.lvl"sv,
        "Data/Levels/Arena/dm6-2.lvl"sv,
        "Data/Levels/Arena/dm6-3.lvl"sv,
        "Data/Levels/Arena/dm6-4.lvl"sv,
        "Data/Levels/Arena/dm6-5.lvl"sv,
        "Data/Levels/Arena/dm7-1.lvl"sv,
        "Data/Levels/Arena/dm7-2.lvl"sv,
        "Data/Levels/Arena/dm7-3.lvl"sv,
        "Data/Levels/Arena/dm7-4.lvl"sv,
        "Data/Levels/Arena/dm7-5.lvl"sv,
        "Data/Levels/Arena/dm8-1.lvl"sv,
        "Data/Levels/Arena/dm8-2.lvl"sv,
        "Data/Levels/Arena/dm8-3.lvl"sv,
        "Data/Levels/Arena/dm8-4.lvl"sv,
        "Data/Levels/Arena/dm8-5.lvl"sv,
    };
    constexpr std::size_t setroom_width{ 10 };
    constexpr std::size_t setroom_height{ 8 };
    constexpr std::size_t preview_width{ 30 };
    constexpr std::size_t preview_height{ 15 };
    using DmPreviewLevel = std::uint8_t[preview_height][preview_width];
    using DmPreviewLevelArray = std::array<DmPreviewLevel, 40>;
    static_assert(arena_levels.size() == DmPreviewLevelArray{}.size());
    static_assert(sizeof(DmPreviewLevelArray) == 18000);

    const std::unordered_map<std::string_view, std::uint8_t> known_preview_images{
        { "empty"sv, 0xFF },
        { "floor"sv, 0x00 },
        { "push_block"sv, 0x01 },
        { "crate"sv, 0x02 },
        { "ladder"sv, 0x03 },
        { "vines"sv, 0x04 },
        { "chain"sv, 0x05 },
        { "pole"sv, 0x06 },
        { "spikes"sv, 0x07 },
        { "ceiling_spikes"sv, 0x08 },
        { "bone_blocks"sv, 0x09 },
        { "spear_trap"sv, 0x0A },
        { "falling_platform"sv, 0x0B },
        { "conveyer_left"sv, 0x0C },
        { "conveyer_right"sv, 0x0D },
        { "tnt"sv, 0x0E },
        { "liquid"sv, 0x0F },
        { "crush_trap"sv, 0x10 },
        { "quick_sand"sv, 0x11 },
        { "ice"sv, 0x12 },
        { "spring"sv, 0x13 },
        { "elevator"sv, 0x14 },
        { "lasers"sv, 0x15 },
        { "spark_balls"sv, 0x16 },
        { "regen_blocks"sv, 0x17 },
        { "tubes"sv, 0x18 },
        { "foliage"sv, 0x19 },
    };
    const std::unordered_map<std::string_view, std::uint8_t> known_tile_codes{
        { "empty"sv, known_preview_images.at("empty"sv) },
        { "bone_block"sv, known_preview_images.at("bone_blocks"sv) },
        { "climbing_pole"sv, known_preview_images.at("pole"sv) },
        { "conveyorbelt_left"sv, known_preview_images.at("conveyer_left"sv) },
        { "conveyorbelt_right"sv, known_preview_images.at("conveyer_right"sv) },
        { "crate"sv, known_preview_images.at("crate"sv) },
        { "crushtrap"sv, known_preview_images.at("crush_trap"sv) },
        { "crushtraplarge"sv, known_preview_images.at("crush_trap"sv) },
        { "elevator"sv, known_preview_images.at("elevator"sv) },
        { "falling_platform"sv, known_preview_images.at("falling_platform"sv) },
        { "floor_hard"sv, known_preview_images.at("empty"sv) },
        { "floor"sv, known_preview_images.at("floor"sv) },
        { "icefloor"sv, known_preview_images.at("ice"sv) },
        { "forcefield_top"sv, known_preview_images.at("lasers"sv) },
        { "forcefield"sv, known_preview_images.at("lasers"sv) },
        { "jungle_spear_trap"sv, known_preview_images.at("spear_trap"sv) },
        { "ladder_plat"sv, known_preview_images.at("ladder"sv) },
        { "ladder"sv, known_preview_images.at("ladder"sv) },
        { "lava"sv, known_preview_images.at("liquid"sv) },
        { "coarse_lava"sv, known_preview_images.at("liquid"sv) },
        { "mushroom_base"sv, known_preview_images.at("foliage"sv) },
        { "pagoda_platform"sv, known_preview_images.at("floor"sv) },
        { "pipe"sv, known_preview_images.at("tubes"sv) },
        { "powder_keg"sv, known_preview_images.at("tnt"sv) },
        { "push_block"sv, known_preview_images.at("push_block"sv) },
        { "quicksand"sv, known_preview_images.at("quick_sand"sv) },
        { "regenerating_block"sv, known_preview_images.at("regen_blocks"sv) },
        { "spark_trap"sv, known_preview_images.at("spark_balls"sv) },
        { "spikes"sv, known_preview_images.at("spikes"sv) },
        { "spring_trap"sv, known_preview_images.at("spring"sv) },
        { "thinice"sv, known_preview_images.at("ice"sv) },
        { "thorn_vine"sv, known_preview_images.at("floor"sv) },
        { "timed_forcefield"sv, known_preview_images.at("lasers"sv) },
        { "tree_base"sv, known_preview_images.at("foliage"sv) },
        { "upsidedown_spikes"sv, known_preview_images.at("ceiling_spikes"sv) },
        { "vine"sv, known_preview_images.at("vines"sv) },
        { "water"sv, known_preview_images.at("liquid"sv) },
        { "chainandblocks_ceiling"sv, known_preview_images.at("floor"sv) },
        { "chain_ceiling"sv, known_preview_images.at("floor"sv) },
        { "factory_generator"sv, known_preview_images.at("floor"sv) },
        { "slidingwall_ceiling"sv, known_preview_images.at("floor"sv) },
        // TODO: Check these
        { "bigspear_trap"sv, known_preview_images.at("empty"sv) },
        { "giantclam"sv, known_preview_images.at("empty"sv) },
        { "giant_frog"sv, known_preview_images.at("empty"sv) },
        { "fountain_drain"sv, known_preview_images.at("empty"sv) },
        { "idol_hold"sv, known_preview_images.at("empty"sv) },
        { "landmine"sv, known_preview_images.at("empty"sv) },
        { "laser_trap"sv, known_preview_images.at("empty"sv) },
        { "slidingwall_switch"sv, known_preview_images.at("empty"sv) },
    };

    const fs::path input_path{ vfs.GetFilePath(dmpreview_tok_path).value_or(source_folder / dmpreview_tok_path) };
    DmPreviewLevelArray level_previews;

    if (auto input_file = std::ifstream{ input_path, std::ios::binary })
    {
        input_file.read((char*)level_previews.data(), sizeof(level_previews));
    }
    else
    {
        return false;
    }

    for (auto [level_path, level_preview] : zip::zip(arena_levels, level_previews))
    {
        if (auto modded_level = vfs.GetFilePath(level_path))
        {
            const LevelData level_data{ LevelParser{}.LoadLevel(modded_level.value()) };
            std::memset(level_preview, 0xff, sizeof(level_preview));
            const std::size_t tiles_width{ level_data.Width * setroom_width };
            const std::size_t tiles_height{ level_data.Height * setroom_height - 1 }; // -1 because last row is always ignored !?!?
            const bool big_level{ level_data.Width == 3 };
            const std::size_t start_x{ big_level ? 0ull : 5ull };
            const std::size_t start_y{ big_level ? 0ull : 2ull };
            for (std::size_t x = 0; x < tiles_width; x++)
            {
                if (start_x + x >= preview_width)
                {
                    continue;
                }

                const std::size_t room_x{ x / setroom_width };
                const std::size_t real_x{ x - room_x * setroom_width };
                for (std::size_t y = 0; y < tiles_height; y++)
                {
                    if (start_y + y >= preview_height)
                    {
                        continue;
                    }

                    const std::size_t room_y{ y / setroom_height };
                    const std::size_t real_y{ y - room_y * setroom_height };
                    if (const LevelRoom* room = algo::find(level_data.Rooms, &LevelRoom::Name, fmt::format("setroom{}-{}", room_y, room_x)))
                    {
                        const std::uint8_t short_tilecode{ room->FrontLayer()[real_x][real_y] };
                        const TileCode* tilecode{ algo::find(level_data.TileCodes, &TileCode::ShortCode, short_tilecode) };
                        if (tilecode != nullptr)
                        {
                            std::string_view placing_tilecode{ tilecode->TileOne };
                            if (room->Flipped() && (placing_tilecode == "conveyer_left"sv || placing_tilecode == "conveyer_right"sv))
                            {
                                placing_tilecode = placing_tilecode == "conveyer_left"sv
                                                       ? "conveyer_right"sv
                                                       : "conveyer_left"sv;
                            }
                            else if (placing_tilecode.contains("floor"sv) && !known_tile_codes.contains(tilecode->TileOne))
                            {
                                placing_tilecode = "floor"sv;
                            }
                            else if (!known_tile_codes.contains(tilecode->TileOne))
                            {
                                continue;
                            }

                            const std::uint8_t preview_image{ known_tile_codes.at(placing_tilecode) };
                            if (preview_image != 0xff)
                            {
                                level_preview[start_y + y][start_x + x] = preview_image;

                                if (placing_tilecode == "tree_base"sv || placing_tilecode == "mushroom_base"sv)
                                {
                                    level_preview[start_y + y - 1][start_x + x] = preview_image;
                                    level_preview[start_y + y - 2][start_x + x] = preview_image;
                                    level_preview[start_y + y - 3][start_x + x] = 0x1A; // mystery tree top
                                }
                                else if (placing_tilecode == "chainandblocks_ceiling"sv || placing_tilecode == "chain_ceiling"sv)
                                {
                                    const std::uint8_t chain_image{ known_preview_images.at("chain"sv) };
                                    level_preview[start_y + y + 1][start_x + x] = chain_image;
                                    level_preview[start_y + y + 2][start_x + x] = chain_image;
                                    level_preview[start_y + y + 3][start_x + x] = chain_image;
                                    level_preview[start_y + y + 4][start_x + x] = chain_image;
                                }
                                else if (placing_tilecode == "crushtraplarge"sv)
                                {
                                    level_preview[start_y + y + 1][start_x + x + 1] = preview_image;
                                    level_preview[start_y + y + 1][start_x + x + 0] = preview_image;
                                    level_preview[start_y + y + 0][start_x + x + 1] = preview_image;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    const fs::path output_path{ destination_folder / dmpreview_tok_path };
    const fs::path output_parent{ output_path.parent_path() };
    if (!fs::exists(output_parent))
    {
        fs::create_directories(output_parent);
    }
    if (auto output_file = std::ofstream{ output_path, std::ios::binary })
    {
        output_file.write((char*)&level_previews, sizeof(level_previews));
    }
    else
    {
        return false;
    }

    /*
    // Note: Keep for potentially debugging
    const auto to_string = [&](const DmPreviewLevel& preview_level)
    {
        std::string as_string{};
        as_string.reserve(sizeof(preview_level) + preview_height);
        for (auto& row : preview_level)
        {
            for (auto& tile : row)
            {
                if (tile == 0xff)
                {
                    as_string.push_back(' ');
                }
                else
                {
                    for (auto& [name, preview_tile] : known_preview_images)
                    {
                        if (preview_tile == tile)
                        {
                            as_string.push_back(name[0]);
                            break;
                        }
                    }
                }
            }
            as_string.push_back('\n');
        }
        as_string.pop_back();
        return as_string;
    };
    if (auto output_file = std::ofstream{ destination_folder / "Data/Levels/Arena/preview.txt" })
    {
        output_file << "Mine" << std::endl;

        for (auto [name, level] : zip::zip(arena_levels, level_previews))
        {
            output_file << name << std::endl;
            output_file << to_string(level);
            output_file << "\n\n";
        }
    }

    const fs::path good_input_path{ vfs.GetFilePath("Data/Levels/Arena/dmpreview_good.tok").value_or(source_folder / "Data/Levels/Arena/dmpreview_good.tok") };
    DmPreviewLevelArray good_level_previews;
    if (auto input_file = std::ifstream{ good_input_path, std::ios::binary })
    {
        input_file.read((char*)good_level_previews.data(), sizeof(good_level_previews));
        if (auto output_file = std::ofstream{ destination_folder / "Data/Levels/Arena/gpreview.txt" })
        {
            output_file << "Known Good" << std::endl;

            for (auto [name, level] : zip::zip(arena_levels, good_level_previews))
            {
                output_file << name << std::endl;
                output_file << to_string(level);
                output_file << "\n\n";
            }
        }
    }
    /**/

    return true;
}
