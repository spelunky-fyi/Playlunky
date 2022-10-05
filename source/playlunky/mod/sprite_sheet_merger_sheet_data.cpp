#include "sprite_sheet_merger.h"

#include "entity_data_extraction.h"
#include "image_processing.h"
#include "util/algorithms.h"
#include "util/format.h"

// Note: All the `TileMap = std::vector<TileMapping>{ ... }` code is because of a bug in MSVC
void SpriteSheetMerger::MakeItemsSheet()
{
    std::vector<SourceSheet> source_sheets;

    source_sheets.push_back(SourceSheet{
        .Path{ "Data/Textures/Entities/turkey_full" },
        .Size{ .Width{ 2048 }, .Height{ 960 } },
        .TileMap = std::vector<TileMapping>{
            TileMapping{
                .SourceTile{ 0, 832, 128, 960 },
                .TargetTile{ 1920, 1792, 2048, 1920 },
            } } });
    source_sheets.push_back(SourceSheet{
        .Path{ "Data/Textures/Entities/Mounts/turkey" },
        .Size{ .Width{ 1024 }, .Height{ 1472 } },
        .TileMap = std::vector<TileMapping>{
            TileMapping{
                .SourceTile{ 0, 1344, 128, 1472 },
                .TargetTile{ 1920, 1792, 2048, 1920 },
            } } });

    m_TargetSheets.push_back(TargetSheet{
        .Path{ "Data/Textures/items" },
        .Size{ .Width{ 2048 }, .Height{ 2048 } },
        .SourceSheets{ std::move(source_sheets) } });
}
void SpriteSheetMerger::MakeJournalItemsSheet()
{
    std::vector<SourceSheet> source_sheets;

    source_sheets.push_back(SourceSheet{
        .Path{ "Data/Textures/Entities/turkey_full" },
        .Size{ .Width{ 2048 }, .Height{ 960 } },
        .TileMap = std::vector<TileMapping>{
            TileMapping{
                .SourceTile{ 0, 672, 160, 832 },
                .TargetTile{ 800, 480, 960, 640 },
            } } });
    source_sheets.push_back(SourceSheet{
        .Path{ "Data/Textures/Entities/Mounts/turkey" },
        .Size{ .Width{ 1024 }, .Height{ 1472 } },
        .TileMap = std::vector<TileMapping>{
            TileMapping{
                .SourceTile{ 0, 1184, 160, 1344 },
                .TargetTile{ 800, 480, 960, 640 },
            } } });

    m_TargetSheets.push_back(TargetSheet{
        .Path{ "Data/Textures/journal_entry_items" },
        .Size{ .Width{ 1600 }, .Height{ 1600 } },
        .SourceSheets{ std::move(source_sheets) } });
}
void SpriteSheetMerger::MakeJournalMonstersSheet()
{
    std::vector<SourceSheet> source_sheets;

    source_sheets.push_back(SourceSheet{
        .Path{ "Data/Textures/Entities/monty_full" },
        .Size{ .Width{ 1536 }, .Height{ 672 } },
        .TileMap = std::vector<TileMapping>{
            TileMapping{
                .SourceTile{ 0, 512, 160, 672 },
                .TargetTile{ 800, 480, 960, 640 },
            } } });
    source_sheets.push_back(SourceSheet{
        .Path{ "Data/Textures/Entities/percy_full" },
        .Size{ .Width{ 1536 }, .Height{ 672 } },
        .TileMap = std::vector<TileMapping>{
            TileMapping{
                .SourceTile{ 0, 512, 160, 672 },
                .TargetTile{ 1440, 480, 1600, 640 },
            } } });
    source_sheets.push_back(SourceSheet{
        .Path{ "Data/Textures/Entities/poochi_full" },
        .Size{ .Width{ 1536 }, .Height{ 672 } },
        .TileMap = std::vector<TileMapping>{
            TileMapping{
                .SourceTile{ 0, 512, 160, 672 },
                .TargetTile{ 640, 480, 800, 640 },
            } } });
    source_sheets.push_back(SourceSheet{
        .Path{ "Data/Textures/Entities/turkey_full" },
        .Size{ .Width{ 2048 }, .Height{ 960 } },
        .TileMap = std::vector<TileMapping>{
            TileMapping{
                .SourceTile{ 0, 512, 160, 672 },
                .TargetTile{ 1280, 640, 1440, 800 },
            } } });
    source_sheets.push_back(SourceSheet{
        .Path{ "Data/Textures/Entities/rockdog_full" },
        .Size{ .Width{ 1920 }, .Height{ 672 } },
        .TileMap = std::vector<TileMapping>{
            TileMapping{
                .SourceTile{ 0, 512, 160, 672 },
                .TargetTile{ 1440, 640, 1600, 800 },
            } } });
    source_sheets.push_back(SourceSheet{
        .Path{ "Data/Textures/Entities/axolotl_full" },
        .Size{ .Width{ 1920 }, .Height{ 672 } },
        .TileMap = std::vector<TileMapping>{
            TileMapping{
                .SourceTile{ 0, 512, 160, 672 },
                .TargetTile{ 0, 800, 160, 960 },
            } } });
    source_sheets.push_back(SourceSheet{
        .Path{ "Data/Textures/Entities/qilin_full" },
        .Size{ .Width{ 1920 }, .Height{ 672 } },
        .TileMap = std::vector<TileMapping>{
            TileMapping{
                .SourceTile{ 0, 512, 160, 672 },
                .TargetTile{ 160, 800, 320, 960 },
            } } });

    {
        struct MonsterJournalEntry
        {
            std::string_view SubPath;
            Tile TargetTile;
        };
        std::vector<MonsterJournalEntry> entries{
            { "Pets/monty", { 800, 480, 960, 640 } },
            { "Pets/percy", { 1440, 480, 1600, 640 } },
            { "Pets/poochi", { 640, 480, 800, 640 } },

            { "Pets/monty_v2", { 800, 480, 960, 640 } },
            { "Pets/percy_v2", { 1440, 480, 1600, 640 } },
            { "Pets/poochi_v2", { 640, 480, 800, 640 } },

            { "Mounts/turkey", { 1280, 640, 1440, 800 } },
            { "Mounts/rockdog", { 1440, 640, 1600, 800 } },
            { "Mounts/axolotl", { 0, 800, 160, 960 } },
            { "Mounts/qilin", { 160, 800, 320, 960 } },

            { "Monsters/snake", { 0, 0, 160, 160 } },
            { "Monsters/spider", { 160, 0, 320, 160 } },
            { "Monsters/hang_spider", { 320, 0, 480, 160 } },
            { "Monsters/bat", { 480, 0, 640, 160 } },
            { "Monsters/cave_man", { 640, 0, 800, 160 } },
            { "Monsters/skeleton", { 800, 0, 960, 160 } },
            { "Monsters/scorpion", { 960, 0, 1120, 160 } },
            { "Monsters/horned_lizard", { 1120, 0, 1280, 160 } },
            { "Monsters/man_trap", { 1280, 0, 1440, 160 } },
            { "Monsters/tiki_man", { 1440, 0, 1600, 160 } },
            { "Monsters/witch_doctor", { 0, 160, 160, 320 } },
            { "Monsters/mosquito", { 160, 160, 320, 320 } },
            { "Monsters/monkey", { 320, 160, 480, 320 } },
            { "Monsters/magmar", { 480, 160, 640, 320 } },
            { "Monsters/robot", { 640, 160, 800, 320 } },
            { "Monsters/fire_bug", { 800, 160, 960, 320 } },
            { "Monsters/imp", { 960, 160, 1120, 320 } },
            { "Monsters/vampire", { 1120, 160, 1280, 320 } },
            { "Monsters/vlad", { 1280, 160, 1440, 320 } },
            { "Monsters/croc_man", { 1440, 160, 1600, 320 } },
            { "Monsters/cobra", { 0, 320, 160, 480 } },
            { "Monsters/sorceress", { 160, 320, 320, 480 } },
            { "Monsters/cat_mummy", { 320, 320, 480, 480 } },
            { "Monsters/jiangshi", { 480, 320, 640, 480 } },
            { "Monsters/flying_fish", { 640, 320, 800, 480 } },
            { "Monsters/octopus", { 800, 320, 960, 480 } },
            { "Monsters/hermit_crab", { 960, 320, 1120, 480 } },
            { "Monsters/ufo", { 1120, 320, 1280, 480 } },
            { "Monsters/alien", { 1280, 320, 1440, 480 } },
            { "Monsters/yeti", { 1440, 320, 1600, 480 } },
            { "Monsters/olmite_naked", { 0, 480, 160, 640 } },
            { "Monsters/necromancer", { 160, 480, 320, 640 } },
            { "Monsters/bee", { 320, 480, 480, 640 } },
            { "Monsters/golden_monkey", { 960, 480, 1120, 640 } },
            { "Monsters/female_jiangshi", { 1120, 480, 1280, 640 } },
            { "Monsters/mole", { 1280, 480, 1440, 640 } },
            { "Monsters/proto_shopkeeper", { 0, 640, 160, 800 } },
            { "Monsters/grub", { 160, 640, 320, 800 } },
            { "Monsters/frog", { 320, 640, 480, 800 } },
            { "Monsters/fire_frog", { 480, 640, 640, 800 } },
            { "Monsters/leprechaun", { 640, 640, 800, 800 } },
            { "Monsters/jumpdog", { 800, 640, 960, 800 } },
            { "Monsters/tadpole", { 1120, 640, 1280, 800 } },

            { "Ghost/ghist", { 960, 640, 1120, 800 } },
        };
        for (const auto& entry : entries)
        {
            if (auto mapping = m_EntityDataExtractor->GetAdditionalMapping(fmt::format("Data/Textures/Entities/{}", entry.SubPath),
                                                                           Tile{ 0, 0, 160, 160 },
                                                                           entry.TargetTile))
            {
                source_sheets.push_back(std::move(mapping).value());
            }
        }
    }

    m_TargetSheets.push_back(TargetSheet{
        .Path{ "Data/Textures/journal_entry_mons" },
        .Size{ .Width{ 1600 }, .Height{ 960 } },
        .SourceSheets{ std::move(source_sheets) } });
}
void SpriteSheetMerger::MakeJournalMonstersBigSheet()
{
    std::vector<SourceSheet> source_sheets;

    {
        struct BigMonsterJournalEntry
        {
            std::string_view SubPath;
            Tile TargetTile;
        };
        std::vector<BigMonsterJournalEntry> entries{
            { "Ghost/ghost", { 320, 320, 640, 640 } },

            { "BigMonsters/quill_back", { 0, 0, 320, 320 } },
            { "BigMonsters/mummy", { 640, 0, 960, 320 } },
            { "BigMonsters/lamassu", { 1280, 0, 1600, 320 } },
            { "BigMonsters/queen_bee", { 0, 320, 320, 640 } },
            { "BigMonsters/yeti_king", { 960, 320, 1280, 640 } },
            { "BigMonsters/yeti_queen", { 1280, 320, 1600, 640 } },
            { "BigMonsters/giant_frog", { 0, 640, 320, 960 } },
            { "BigMonsters/giant_spider", { 320, 640, 640, 960 } },
            { "BigMonsters/lavamander", { 640, 640, 960, 960 } },
            { "BigMonsters/crab_man", { 960, 640, 1280, 960 } },
            { "BigMonsters/eggplant_minister", { 0, 960, 320, 1280 } },
            { "BigMonsters/giant_fish", { 640, 960, 960, 1280 } },
            { "BigMonsters/alien_queen", { 960, 960, 1280, 1280 } },
            { "BigMonsters/ammit", { 1280, 960, 1600, 1280 } },
            { "BigMonsters/osiris", { 960, 1280, 1280, 1600 } },
            { "BigMonsters/giant_fly", { 0, 1600, 320, 1920 } },
            //{ "BigMonsters/olmec", { 320, 0, 640, 320 } },
            //{ "BigMonsters/celestial_jellyfish", { 1280, 640, 1600, 960 } },
            //{ "Mounts/mech", { 320, 960, 640, 1280 } },
            //{ "BigMonsters/anubis", { 960, 0, 1280, 320 } },
            //{ "BigMonsters/anubis_2", { 0, 1280, 320, 1600 } },
            //{ "BigMonsters/hundun", { 320, 1280, 640, 1600 } },
            //{ "BigMonsters/kingu", { 640, 1280, 960, 1600 } },
            //{ "BigMonsters/tiamat", { 1280, 1280, 1600, 1600 } },
        };
        for (const auto& entry : entries)
        {
            if (auto mapping = m_EntityDataExtractor->GetAdditionalMapping(fmt::format("Data/Textures/Entities/{}", entry.SubPath),
                                                                           Tile{ 0, 0, 320, 320 },
                                                                           entry.TargetTile))
            {
                source_sheets.push_back(std::move(mapping).value());
            }
        }
    }

    m_TargetSheets.push_back(TargetSheet{
        .Path{ "Data/Textures/journal_entry_mons_big" },
        .Size{ .Width{ 1600 }, .Height{ 1920 } },
        .SourceSheets{ std::move(source_sheets) },
        .RandomSelect{ false } });
}
void SpriteSheetMerger::MakeJournalPeopleSheet(bool force_regen)
{
    std::vector<SourceSheet> source_sheets;

    {
        std::uint32_t char_x{ 0 };
        std::uint32_t char_y{ 0 };
        for (std::string_view color : { "yellow", "magenta", "cyan", "black", "cinnabar", "green", "olive", "white", "cerulean", "blue", "lime", "lemon", "iris", "gold", "red", "pink", "violet", "gray", "khaki", "orange" })
        {
            source_sheets.push_back(SourceSheet{
                .Path{ fmt::format("Data/Textures/Entities/char_{}_full", color) },
                .Size{ .Width{ 2048 }, .Height{ 2160 } },
                .TileMap = std::vector<TileMapping>{
                    TileMapping{
                        .SourceTile{ 0, 1920, 160, 2080 },
                        .TargetTile{ char_x * 160, char_y * 160, char_x * 160 + 160, char_y * 160 + 160 },
                    } } });
            source_sheets.push_back(source_sheets.back());
            source_sheets.back().Size.Height = 2224;
            if (mGenerateCharacterJournalEntriesEnabled)
            {
                source_sheets.push_back(SourceSheet{
                    .Path{ fmt::format("Data/Textures/char_{}", color) },
                    .Size{ .Width{ 2048 }, .Height{ 2048 } },
                    .TileMap = std::vector<TileMapping>{
                        TileMapping{
                            .SourceTile{ 0, 0, 128, 128 },
                            .TargetTile{ char_x * 160, char_y * 160, char_x * 160 + 160, char_y * 160 + 160 },
                        } } });
            }
            char_x++;
            if (char_x >= 10)
            {
                char_x = 0;
                char_y++;
            }
        }
    }

    source_sheets.push_back(SourceSheet{
        .Path{ "Data/Textures/Entities/char_eggchild_full" },
        .Size{ .Width{ 2048 }, .Height{ 2080 } },
        .TileMap = std::vector<TileMapping>{
            TileMapping{
                .SourceTile{ 0, 1920, 160, 2080 },
                .TargetTile{ 160, 320, 320, 480 },
            } } });
    if (mGenerateCharacterJournalEntriesEnabled)
    {
        source_sheets.push_back(SourceSheet{
            .Path{ "Data/Textures/char_eggchild" },
            .Size{ .Width{ 2048 }, .Height{ 2048 } },
            .TileMap = std::vector<TileMapping>{
                TileMapping{
                    .SourceTile{ 0, 0, 128, 128 },
                    .TargetTile{ 160, 320, 320, 480 },
                } } });
    }
    source_sheets.push_back(SourceSheet{
        .Path{ "Data/Textures/Entities/char_hired_full" },
        .Size{ .Width{ 2048 }, .Height{ 2080 } },
        .TileMap = std::vector<TileMapping>{
            TileMapping{
                .SourceTile{ 0, 1920, 160, 2080 },
                .TargetTile{ 0, 320, 160, 480 },
            } } });
    if (mGenerateCharacterJournalEntriesEnabled)
    {
        source_sheets.push_back(SourceSheet{
            .Path{ "Data/Textures/Entities/char_hired" },
            .Size{ .Width{ 2048 }, .Height{ 2048 } },
            .TileMap = std::vector<TileMapping>{
                TileMapping{
                    .SourceTile{ 0, 0, 128, 128 },
                    .TargetTile{ 0, 320, 160, 480 },
                } } });
    }

    {
        struct PeopleJournalEntry
        {
            std::string_view SubPath;
            Tile TargetTile;
        };
        std::vector<PeopleJournalEntry> entries{
            { "Ghost/ghist", { 480, 480, 640, 640 } },

            { "Monsters/cave_man", { 320, 480, 480, 640 } },

            { "People/shopkeeper", { 320, 320, 480, 480 } },
            { "People/merchant", { 480, 320, 640, 480 } },
            { "People/yang", { 640, 320, 800, 480 } },
            { "People/old_hunter", { 800, 320, 960, 480 } },
            { "People/thief", { 960, 320, 1120, 480 } },
            { "People/parsley", { 1120, 320, 1280, 480 } },
            { "People/parsnip", { 1280, 320, 1440, 480 } },
            { "People/parmesan", { 1440, 320, 1600, 480 } },
            { "People/hunduns_servant", { 0, 480, 160, 640 } },
            { "People/bodyguard", { 160, 480, 320, 640 } },
            //{ "People/mama_tunnel", { 0, 640, 160, 800 } },
        };
        for (const auto& entry : entries)
        {
            if (auto mapping = m_EntityDataExtractor->GetAdditionalMapping(fmt::format("Data/Textures/Entities/{}", entry.SubPath),
                                                                           Tile{ 0, 0, 160, 160 },
                                                                           entry.TargetTile))
            {
                source_sheets.push_back(std::move(mapping).value());
            }
        }
    }

    {
        struct BigPeopleJournalEntry
        {
            std::string_view SubPath;
            Tile TargetTile;
        };
        std::vector<BigPeopleJournalEntry> entries{
            //{ "BigMonsters/yama", { 640, 480, 960, 800 } },
            { "BigMonsters/madame_tusk", { 960, 480, 1280, 800 } },
            { "BigMonsters/waddler", { 1280, 480, 1600, 800 } },
        };
        for (const auto& entry : entries)
        {
            if (auto mapping = m_EntityDataExtractor->GetAdditionalMapping(fmt::format("Data/Textures/Entities/{}", entry.SubPath),
                                                                           Tile{ 0, 0, 320, 320 },
                                                                           entry.TargetTile))
            {
                source_sheets.push_back(std::move(mapping).value());
            }
        }
    }

    m_TargetSheets.push_back(TargetSheet{
        .Path{ "Data/Textures/journal_entry_people" },
        .Size{ .Width{ 1600 }, .Height{ 800 } },
        .SourceSheets{ std::move(source_sheets) },
        .RandomSelect{ mRandomCharacterSelectEnabled },
        .ForceRegen{ force_regen } });
}
void SpriteSheetMerger::MakeJournalStickerSheet(bool force_regen)
{
    std::vector<SourceSheet> source_sheets;

    {
        std::uint32_t char_x{ 0 };
        std::uint32_t char_y{ 0 };
        for (std::string_view color : { "yellow", "magenta", "cyan", "black", "cinnabar", "green", "olive", "white", "cerulean", "blue", "lime", "lemon", "iris", "gold", "red", "pink", "violet", "gray", "khaki", "orange" })
        {
            source_sheets.push_back(SourceSheet{
                .Path{ fmt::format("Data/Textures/Entities/char_{}_full", color) },
                .Size{ .Width{ 2048 }, .Height{ 2160 } },
                .TileMap = std::vector<TileMapping>{
                    TileMapping{
                        .SourceTile{ 0, 2080, 80, 2160 },
                        .TargetTile{ char_x * 80, char_y * 80, char_x * 80 + 80, char_y * 80 + 80 },
                    } } });
            source_sheets.push_back(source_sheets.back());
            source_sheets.back().Size.Height = 2224;
            if (mGenerateCharacterJournalStickersEnabled)
            {
                source_sheets.push_back(SourceSheet{
                    .Path{ fmt::format("Data/Textures/char_{}", color) },
                    .Size{ .Width{ 2048 }, .Height{ 2048 } },
                    .TileMap = std::vector<TileMapping>{
                        TileMapping{
                            .SourceTile{ 256, 1152, 384, 1280 },
                            .TargetTile{ char_x * 80, char_y * 80, char_x * 80 + 80, char_y * 80 + 80 },
                        } },
                    .Processing{ mGenerateStickerPixelArtEnabled ? GenerateStickerPixelArt : nullptr } });
            }
            char_x++;
            if (char_x >= 10)
            {
                char_x = 0;
                char_y++;
            }
        }
    }

    {
        struct StickerEntry
        {
            std::string_view SubPath;
            Tile TargetTile;
        };
        std::vector<StickerEntry> entries{
            { "Monsters/cave_man", { 160, 400, 240, 480 } },
            { "Monsters/vlad", { 720, 400, 800, 480 } },

            { "People/shopkeeper", { 0, 400, 80, 480 } },
            { "People/merchant", { 80, 400, 160, 480 } },
            { "People/yang", { 240, 400, 320, 480 } },
            { "People/parsley", { 320, 400, 400, 480 } },
            { "People/parsnip", { 400, 400, 480, 480 } },
            { "People/parmesan", { 480, 400, 560, 480 } },
            { "People/old_hunter", { 560, 400, 640, 480 } },
            { "People/thief", { 640, 400, 720, 480 } },
        };
        for (const auto& entry : entries)
        {
            if (auto mapping = m_EntityDataExtractor->GetAdditionalMapping(fmt::format("Data/Textures/Entities/{}", entry.SubPath),
                                                                           Tile{ 0, 160, 80, 240 },
                                                                           entry.TargetTile))
            {
                source_sheets.push_back(std::move(mapping).value());
            }
        }
    }

    {
        struct BigStickerEntry
        {
            std::string_view SubPath;
            Tile TargetTile;
        };
        std::vector<BigStickerEntry> entries{
            { "BigMonsters/quill_back", { 0, 480, 160, 640 } },
            //{ "BigMonsters/kingu", { 160, 480, 320, 640 } },
            //{ "BigMonsters/anubis", { 320, 480, 480, 640 } },
            { "BigMonsters/osiris", { 480, 480, 640, 640 } },
            { "BigMonsters/alien_queen", { 640, 480, 800, 640 } },
            //{ "BigMonsters/olmec", { 0, 640, 160, 800 } },
            //{ "BigMonsters/tiamat", { 160, 640, 320, 800 } },
            //{ "BigMonsters/yama", { 320, 640, 480, 800 } },
            //{ "BigMonsters/hundun", { 480, 640, 640, 800 } },
        };
        for (const auto& entry : entries)
        {
            if (auto mapping = m_EntityDataExtractor->GetAdditionalMapping(fmt::format("Data/Textures/Entities/{}", entry.SubPath),
                                                                           Tile{ 0, 320, 160, 480 },
                                                                           entry.TargetTile))
            {
                source_sheets.push_back(std::move(mapping).value());
            }
        }
    }

    m_TargetSheets.push_back(TargetSheet{
        .Path{ "Data/Textures/journal_stickers" },
        .Size{ .Width{ 800 }, .Height{ 800 } },
        .SourceSheets{ std::move(source_sheets) },
        .RandomSelect{ mRandomCharacterSelectEnabled },
        .ForceRegen{ force_regen } });
}
void SpriteSheetMerger::MakeMountsTargetSheet()
{
    std::vector<std::pair<std::string_view, std::uint32_t>> name_to_idx{
        { "turkey", 0 },
        { "rockdog", 1 },
        { "axolotl", 2 },
        { "qilin", 3 }
    };

    std::vector<SourceSheet> source_sheets;
    for (const auto& [mount_name, idx] : name_to_idx)
    {
        const std::uint32_t image_width = mount_name == "turkey" ? 2048 : 1920;
        const std::uint32_t image_height = mount_name == "turkey" ? 960 : 672;
        source_sheets.push_back(SourceSheet{
            .Path{ fmt::format("Data/Textures/Entities/{}_full", mount_name) },
            .Size{ .Width{ image_width }, .Height{ image_height } },
            .TileMap = std::vector<TileMapping>{
                TileMapping{
                    .SourceTile{ 0, 0, 2048, 512 },
                    .TargetTile{ 0, 512 * idx, 2048, 512 * idx + 512 },
                } } });
        if (auto sheet = m_EntityDataExtractor->GetEntitySourceSheet(fmt::format("Data/Textures/Entities/Mounts/{}", mount_name)))
        {
            if (mount_name == "turkey")
            {
                sheet->TileMap.push_back(TileMapping{ .SourceTile{ 0, 0, 512, 128 }, .TargetTile{ 1536, 128, 2048, 256 } });
            }
            source_sheets.push_back(std::move(sheet).value());
        }
    }

    m_TargetSheets.push_back(TargetSheet{
        .Path{ "Data/Textures/mounts" },
        .Size{ .Width{ 2048 }, .Height{ 2048 } },
        .SourceSheets{ std::move(source_sheets) } });
}
void SpriteSheetMerger::MakePetsTargetSheet()
{
    std::vector<std::pair<std::string_view, std::uint32_t>> name_to_idx{
        { "monty", 0 },
        { "percy", 1 },
        { "poochi", 2 }
    };

    std::vector<SourceSheet> source_sheets;
    for (const auto& [pet_name, idx] : name_to_idx)
    {
        source_sheets.push_back(SourceSheet{
            .Path{ fmt::format("Data/Textures/Entities/{}_full", pet_name) },
            .Size{ .Width{ 1536 }, .Height{ 672 } },
            .TileMap = std::vector<TileMapping>{
                TileMapping{
                    .SourceTile{ 0, 0, 1536, 512 },
                    .TargetTile{ 0, 512 * idx, 1536, 512 * idx + 512 },
                } } });
        if (auto sheet = m_EntityDataExtractor->GetEntitySourceSheet(fmt::format("Data/Textures/Entities/Pets/{}", pet_name)))
        {
            source_sheets.push_back(std::move(sheet).value());
        }
        if (auto sheet = m_EntityDataExtractor->GetEntitySourceSheet(fmt::format("Data/Textures/Entities/Pets/{}_v2", pet_name)))
        {
            source_sheets.push_back(std::move(sheet).value());
        }
    }

    m_TargetSheets.push_back(TargetSheet{
        .Path{ "Data/Textures/monsters_pets" },
        .Size{ .Width{ 1536 }, .Height{ 1536 } },
        .SourceSheets{ std::move(source_sheets) } });
}
void SpriteSheetMerger::MakeMonstersTargetSheet()
{
    struct AdditionalMapping
    {
        std::string_view SourceFile;
        std::vector<TileMapping> TileMap;
    };
    struct MonsterSheet
    {
        std::string_view TargetFile;
        std::vector<std::string_view> SourceFiles;
        std::vector<AdditionalMapping> AdditionalTileMaps;
        std::vector<SourceSheet> AdditionalSources;
        SheetSize TargetSize{ .Width{ 2048 }, .Height{ 2048 } };
    };
    std::vector<MonsterSheet> sheets{
        { "Data/Textures/monstersbasic01", {
                                               "Monsters/snake",
                                               "Monsters/bat",
                                               "Monsters/fly",
                                               "Monsters/skeleton",
                                               "Monsters/spider",
                                               "Monsters/ufo",
                                               "Monsters/alien",
                                               "Monsters/cobra",
                                               "Monsters/scorpion",
                                               "Monsters/golden_monkey",
                                               "Monsters/bee",
                                               "Monsters/magmar",

                                               "People/shopkeeper",
                                           },
          { { "Monsters/golden_monkey", { TileMapping{ .SourceTile{ 0, 0, 128, 128 }, .TargetTile{ 1408, 1280, 1536, 1408 } }, TileMapping{ .SourceTile{ 0, 0, 128, 128 }, .TargetTile{ 1920, 1280, 2048, 1408 } } } } } },
        { "Data/Textures/monstersbasic02", {
                                               "Monsters/vampire",
                                               "Monsters/vlad",
                                               "Monsters/cave_man",
                                               "Monsters/leprechaun",

                                               "People/bodyguard",
                                               "People/old_hunter",
                                               "People/merchant",
                                           },
          { { "Monsters/cave_man", { TileMapping{ .SourceTile{ 0, 0, 1024, 128 }, .TargetTile{ 0, 896, 1024, 1024 } }, TileMapping{ .SourceTile{ 0, 128, 256, 256 }, .TargetTile{ 1024, 896, 1280, 1024 } } } } } },
        { "Data/Textures/monstersbasic03", {
                                               "Critters/birdies",

                                               "People/hunduns_servant",
                                               "People/thief",
                                               "People/parmesan",
                                               "People/parsley",
                                               "People/parsnip",
                                               "People/yang",
                                           } },
        { "Data/Textures/monsters01", {
                                          "Critters/snail",
                                          "Critters/dung_beetle",
                                          "Critters/butterfly",

                                          "Monsters/robot",
                                          "Monsters/imp",
                                          "Monsters/tiki_man",
                                          "Monsters/man_trap",
                                          "Monsters/fire_bug",
                                          "Monsters/mole",
                                          "Monsters/witch_doctor",
                                          "Monsters/horned_lizard",
                                          "Monsters/witch_doctor_skull",
                                          "Monsters/monkey",
                                          "Monsters/hang_spider",
                                          "Monsters/mosquito",
                                      },
          { { "Monsters/witch_doctor", { TileMapping{ .SourceTile{ 0, 0, 256, 128 }, .TargetTile{ 1536, 1280, 1792, 1408 } } } } } },
        { "Data/Textures/monsters02", {
                                          "Critters/crab",
                                          "Critters/fish",
                                          "Critters/anchovy",
                                          "Critters/locust",

                                          "Monsters/jiangshi",
                                          "Monsters/hermit_crab",
                                          "Monsters/flying_fish",
                                          "Monsters/octopus",
                                          "Monsters/female_jiangshi",
                                          "Monsters/croc_man",
                                          "Monsters/sorceress",
                                          "Monsters/cat_mummy",
                                          "Monsters/necromancer",
                                      },
          { { "Monsters/jiangshi", { TileMapping{ .SourceTile{ 0, 0, 128, 128 }, .TargetTile{ 1152, 0, 1280, 128 } } } }, { "Monsters/female_jiangshi", { TileMapping{ .SourceTile{ 0, 0, 128, 128 }, .TargetTile{ 1152, 768, 1280, 896 } } } }, { "Monsters/hermit_crab", { TileMapping{ .SourceTile{ 0, 0, 768, 128 }, .TargetTile{ 1280, 512, 2048, 640 } }, TileMapping{ .SourceTile{ 768, 0, 896, 128 }, .TargetTile{ 1408, 640, 1536, 768 } }, TileMapping{ .SourceTile{ 0, 128, 512, 256 }, .TargetTile{ 1536, 640, 2048, 768 } } } } },
          std::vector<SourceSheet>{ SourceSheet{ .Path{ "Data/Textures/Entities/Critters/blue_crab" }, .Size{ 384, 256 }, .TileMap = std::vector<TileMapping>{ { { 0, 0, 384, 128 }, { 768, 1664, 1152, 1792 } }, { { 0, 128, 384, 256 }, { 1152, 1664, 1526, 1792 } } } } } },
        { "Data/Textures/monsters03", {
                                          "Critters/firefly",
                                          "Critters/penguin",
                                          "Critters/drone",
                                          "Critters/slime",

                                          "Monsters/yeti",
                                          "Monsters/proto_shopkeeper",
                                          "Monsters/jumpdog",
                                          "Monsters/tadpole",
                                          "Monsters/olmite_naked",
                                          "Monsters/grub",
                                          "Monsters/frog",
                                          "Monsters/fire_frog",
                                      },
          { { "Monsters/fire_frog", { TileMapping{ .SourceTile{ 0, 0, 256, 128 }, .TargetTile{ 640, 1538, 896, 1664 } } } } },
          {
              SourceSheet{ .Path{ "Data/Textures/Entities/Monsters/olmite_armored" }, .Size{ 512, 384 }, .TileMap = std::vector<TileMapping>{
                                                                                                             { { 0, 0, 512, 128 }, { 0, 1024, 512, 1152 } },
                                                                                                             { { 0, 128, 512, 256 }, { 512, 1024, 1024, 1152 } },
                                                                                                             { { 0, 256, 256, 384 }, { 1024, 1024, 1280, 1152 } },
                                                                                                         } },
              SourceSheet{ .Path{ "Data/Textures/Entities/Monsters/olmite_helmet" }, .Size{ 512, 640 }, .TileMap = std::vector<TileMapping>{ { { 0, 0, 512, 128 }, { 0, 1152, 512, 1280 } }, { { 0, 128, 512, 256 }, { 512, 1152, 1024, 1280 } }, { { 0, 256, 256, 384 }, { 1024, 1152, 1280, 1280 } }, { { 256, 256, 512, 384 }, { 0, 1280, 256, 1408 } }, { { 0, 384, 512, 512 }, { 256, 1280, 768, 1408 } }, { { 0, 512, 128, 640 }, { 768, 1280, 896, 1408 } } } },
          } },
    };

    for (const auto& sheet : sheets)
    {
        std::vector<SourceSheet> source_sheets = sheet.AdditionalSources;
        for (std::string_view source_file : sheet.SourceFiles)
        {
            if (auto mapping = m_EntityDataExtractor->GetEntitySourceSheet(fmt::format("Data/Textures/Entities/{}", source_file)))
            {
                if (auto additional_mapping = algo::find(sheet.AdditionalTileMaps, &AdditionalMapping::SourceFile, source_file))
                {
                    mapping->TileMap.insert(mapping->TileMap.end(), additional_mapping->TileMap.begin(), additional_mapping->TileMap.end());
                }
                source_sheets.push_back(std::move(mapping).value());
            }
        }
        m_TargetSheets.push_back(TargetSheet{
            .Path{ sheet.TargetFile },
            .Size{ sheet.TargetSize },
            .SourceSheets{ std::move(source_sheets) },
            .RandomSelect{ false } });
    }
}
void SpriteSheetMerger::MakeBigMonstersTargetSheet()
{
    struct AdditionalMapping
    {
        std::string_view SourceFile;
        std::vector<TileMapping> TileMap;
    };
    struct BigMonsterSheet
    {
        std::string_view TargetFile;
        std::vector<std::string_view> SourceFiles;
        std::vector<AdditionalMapping> AdditionalTileMaps;
        SheetSize TargetSize{ .Width{ 2048 }, .Height{ 2048 } };
    };
    std::vector<BigMonsterSheet> sheets{
        { "Data/Textures/monstersbig01", {
                                             "BigMonsters/quill_back",
                                             "BigMonsters/giant_spider",
                                             "BigMonsters/queen_bee",
                                         },
          { { "BigMonsters/giant_spider", { TileMapping{ .SourceTile{ 0, 0, 256, 256 }, .TargetTile{ 1792, 1280, 2048, 1536 } } } } } },
        { "Data/Textures/monstersbig02", {
                                             "BigMonsters/mummy",
                                         } },
        { "Data/Textures/monstersbig03", {
                                             "BigMonsters/lamassu",
                                             "BigMonsters/yeti_king",
                                             "BigMonsters/yeti_queen",
                                         } },
        { "Data/Textures/monstersbig04", {
                                             "BigMonsters/crab_man",
                                             "BigMonsters/lavamander",
                                             "BigMonsters/giant_fly",
                                             "BigMonsters/giant_clam",
                                         },
          { { "BigMonsters/crab_man", { TileMapping{ .SourceTile{ 0, 0, 256, 256 }, .TargetTile{ 512, 512, 768, 768 } }, TileMapping{ .SourceTile{ 256, 0, 512, 256 }, .TargetTile{ 1792, 256, 2048, 512 } } } }, { "BigMonsters/lavamander", { TileMapping{ .SourceTile{ 0, 0, 768, 256 }, .TargetTile{ 1280, 1280, 2048, 1536 } } } } } },
        { "Data/Textures/monstersbig05", {
                                             "BigMonsters/ammit",
                                             "BigMonsters/madame_tusk",
                                             "BigMonsters/eggplant_minister",
                                             "BigMonsters/giant_frog",
                                         },
          { { "BigMonsters/eggplant_minister", { TileMapping{ .SourceTile{ 0, 0, 512, 128 }, .TargetTile{ 1152, 896, 1664, 1024 } }, TileMapping{ .SourceTile{ 0, 128, 384, 256 }, .TargetTile{ 1664, 896, 2048, 1024 } } } } } },
        { "Data/Textures/monstersbig06", {
                                             "BigMonsters/waddler",
                                             "BigMonsters/giant_fish",
                                         } },
        { "Data/Textures/monsters_osiris", {
                                               "BigMonsters/alien_queen",
                                               "BigMonsters/osiris",
                                           } },
        { "Data/Textures/monsters_ghost", {
                                              "Ghost/ghost_small_surprised",
                                              "Ghost/ghist",
                                              "Ghost/ghost",
                                              "Ghost/ghost_happy",
                                              "Ghost/ghost_sad",
                                              "Ghost/ghost_small_angry",
                                              "Ghost/ghost_small_happy",
                                              "Ghost/ghost_small_sad",
                                          },
          {
              { "Ghost/ghist", { TileMapping{ .SourceTile{ 0, 0, 384, 128 }, .TargetTile{ 896, 1408, 1280, 1536 } }, TileMapping{ .SourceTile{ 0, 128, 384, 258 }, .TargetTile{ 1280, 1408, 1664, 1536 } }, TileMapping{ .SourceTile{ 0, 256, 128, 384 }, .TargetTile{ 1664, 1408, 1792, 1536 } } } },
          } },
    };

    for (const auto& sheet : sheets)
    {
        std::vector<SourceSheet> source_sheets;
        for (std::string_view source_file : sheet.SourceFiles)
        {
            if (auto mapping = m_EntityDataExtractor->GetEntitySourceSheet(fmt::format("Data/Textures/Entities/{}", source_file)))
            {
                if (auto additional_mapping = algo::find(sheet.AdditionalTileMaps, &AdditionalMapping::SourceFile, source_file))
                {
                    mapping->TileMap.insert(mapping->TileMap.end(), additional_mapping->TileMap.begin(), additional_mapping->TileMap.end());
                }
                source_sheets.push_back(std::move(mapping).value());
            }
        }
        m_TargetSheets.push_back(TargetSheet{
            .Path{ sheet.TargetFile },
            .Size{ sheet.TargetSize },
            .SourceSheets{ std::move(source_sheets) },
            .RandomSelect{ false } });
    }
}
void SpriteSheetMerger::MakeCharacterTargetSheet(std::string_view color)
{
    const bool is_npc = color == "hired" || color == "eggchild";
    const std::uint32_t old_image_height = is_npc ? 2080 : 2160;
    std::vector<SourceSheet> source_sheets{
        SourceSheet{
            .Path{ fmt::format("Data/Textures/Entities/char_{}_full", color) },
            .Size{ .Width{ 2048 }, .Height{ old_image_height } },
            .TileMap = std::vector<TileMapping>{
                TileMapping{
                    .SourceTile{ 0, 0, 2048, 1920 },
                    .TargetTile{ 0, 0, 2048, 1920 },
                } } },
        SourceSheet{ .Path{ fmt::format("Data/Textures/char_{}", color) }, .Size{ .Width{ 2048 }, .Height{ 2048 } }, .TileMap = std::vector<TileMapping>{ TileMapping{
                                                                                                                         .SourceTile{ 0, 0, 2048, 1920 },
                                                                                                                         .TargetTile{ 0, 0, 2048, 1920 },
                                                                                                                     } } }
    };
    source_sheets.push_back(source_sheets.front());
    source_sheets.back().Size.Height = 2224;
    m_TargetSheets.push_back(TargetSheet{
        .Path{ fmt::format("Data/Textures/char_{}", color) },
        .Size{ .Width{ 2048 }, .Height{ 2048 } },
        .SourceSheets{ std::move(source_sheets) },
        .RandomSelect{ mRandomCharacterSelectEnabled } });
}
void SpriteSheetMerger::MakeMenuLeaderTargetSheet()
{
    std::vector<SourceSheet> source_sheets;

    {
        std::uint32_t char_x{ 0 };
        std::uint32_t char_y{ 0 };
        for (std::string_view color : { "yellow", "magenta", "cyan", "black", "cinnabar", "green", "olive", "white", "cerulean", "blue", "lime", "lemon", "iris", "gold", "red", "pink", "violet", "gray", "khaki", "orange" })
        {
            source_sheets.push_back(SourceSheet{
                .Path{ fmt::format("Data/Textures/Entities/char_{}_full", color) },
                .Size{ .Width{ 2048 }, .Height{ 2224 } },
                .TileMap = std::vector<TileMapping>{
                    TileMapping{
                        .SourceTile{ 0, 2160, 128, 2224 },
                        .TargetTile{ char_x * 128, 448 + char_y * 128, char_x * 128 + 128, 448 + char_y * 128 + 64 },
                    } } });
            char_x++;
            if (char_x >= 10)
            {
                char_x = 0;
                char_y++;
            }
        }
    }

    m_TargetSheets.push_back(TargetSheet{
        .Path{ "Data/Textures/menu_leader" },
        .Size{ .Width{ 1280 }, .Height{ 1280 } },
        .SourceSheets{ std::move(source_sheets) },
        .RandomSelect{ mRandomCharacterSelectEnabled } });
}
void SpriteSheetMerger::MakeMenuBasicTargetSheet()
{
    std::vector<SourceSheet> source_sheets;
    MultiSourceTile head_tile{
        .Processing{ MakeCombinedMenuPetHeads }
    };

    {
        struct MonsterJournalEntry
        {
            std::string_view SubPath;
            Tile SourceTile;
            Tile TargetTile;
            bool Head;
        };
        std::vector<MonsterJournalEntry> entries{
            { "Pets/monty_v2", { 0, 160, 64, 224 }, { 1024, 448, 1088, 512 } },
            { "Pets/percy_v2", { 0, 160, 64, 224 }, { 1088, 448, 1152, 512 } },
            { "Pets/poochi_v2", { 0, 160, 64, 224 }, { 1152, 448, 1216, 512 } },

            { "Pets/monty_v2", { 0, 224, 128, 352 }, { 1216, 448, 1280, 512 }, true },
            { "Pets/percy_v2", { 0, 224, 128, 352 }, { 1216, 448, 1280, 512 }, true },
            { "Pets/poochi_v2", { 0, 224, 128, 352 }, { 1216, 448, 1280, 512 }, true },
        };
        for (const auto& entry : entries)
        {
            if (auto mapping = m_EntityDataExtractor->GetAdditionalMapping(fmt::format("Data/Textures/Entities/{}", entry.SubPath),
                                                                           entry.SourceTile,
                                                                           entry.TargetTile))
            {

                if (entry.Head)
                {
                    assert(mapping.value().TileMap.size() == 1);

                    head_tile.Paths.push_back(std::move(mapping.value().Path));
                    head_tile.Sizes.push_back(mapping.value().Size);
                    head_tile.TileMap.push_back(mapping.value().TileMap[0]);
                }
                else
                {
                    source_sheets.push_back(std::move(mapping).value());
                }
            }
        }
    }

    m_TargetSheets.push_back(TargetSheet{
        .Path{ "Data/Textures/menu_basic" },
        .Size{ .Width{ 1280 }, .Height{ 1280 } },
        .SourceSheets{ std::move(source_sheets) },
        .MultiSourceTiles{ std::move(head_tile) } });
}
void SpriteSheetMerger::MakeCaveDecoTargetSheet()
{
    std::vector<SourceSheet> source_sheets{
        SourceSheet{
            .Path{ "Data/Textures/Entities/Decorations/udjat_wall_heads" },
            .Size{ .Width{ 512 }, .Height{ 512 } },
            .TileMap = std::vector<TileMapping>{
                TileMapping{
                    .SourceTile{ 0, 0, 512, 512 },
                    .TargetTile{ 1024, 1024, 1536, 1536 },
                } } },
    };
    m_TargetSheets.push_back(TargetSheet{
        .Path{ "Data/Textures/deco_cave" },
        .Size{ .Width{ 1536 }, .Height{ 1536 } },
        .SourceSheets{ std::move(source_sheets) } });
}
