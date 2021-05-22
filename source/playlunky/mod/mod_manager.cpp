#include "mod_manager.h"

#include "cache_audio_file.h"
#include "decode_audio_file.h"
#include "extract_game_assets.h"
#include "fix_mod_structure.h"
#include "mod_database.h"
#include "mod_info.h"
#include "patch_character_definitions.h"
#include "playlunky.h"
#include "playlunky_settings.h"
#include "png_dds_conversion.h"
#include "shader_merge.h"
#include "sprite_sheet_merger.h"
#include "string_hash.h"
#include "string_merge.h"
#include "unzip_mod.h"
#include "virtual_filesystem.h"

#include "log.h"
#include "util/algorithms.h"
#include "util/function_pointer.h"
#include "util/regex.h"

#include "detour/imgui.h"

#include <spel2.h>

#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <unordered_map>
#include <zip.h>

static constexpr ctll::fixed_string s_CharacterRule{ ".+char_(.*)\\.png" };
static constexpr ctll::fixed_string s_StringFileRule{ "strings([0-9]{2})\\.str" };
static constexpr ctll::fixed_string s_StringModFileRule{ "strings([0-9]{2})_mod\\.str" };

static constexpr std::string_view s_SpeedrunFiles[]{
    "Data/Textures/char_orange.DDS",
    "Data/Textures/char_orange.png",
    "Data/Textures/Entities/char_orange_full.png",
    "Data/Textures/char_pink.DDS",
    "Data/Textures/char_pink.png",
    "Data/Textures/Entities/char_pink_full.png",
    "Data/Textures/char_red.DDS",
    "Data/Textures/char_red.png",
    "Data/Textures/Entities/char_red_full.png",
    "Data/Textures/char_violet.DDS",
    "Data/Textures/char_violet.png",
    "Data/Textures/Entities/char_violet_full.png",
    "Data/Textures/char_white.DDS",
    "Data/Textures/char_white.png",
    "Data/Textures/Entities/char_white_full.png",
    "Data/Textures/char_yellow.DDS",
    "Data/Textures/char_yellow.png",
    "Data/Textures/Entities/char_yellow_full.png",
    "Data/Textures/char_black.DDS",
    "Data/Textures/char_black.png",
    "Data/Textures/Entities/char_black_full.png",
    "Data/Textures/char_blue.DDS",
    "Data/Textures/char_blue.png",
    "Data/Textures/Entities/char_blue_full.png",
    "Data/Textures/char_cerulean.DDS",
    "Data/Textures/char_cerulean.png",
    "Data/Textures/Entities/char_cerulean_full.png",
    "Data/Textures/char_cinnabar.DDS",
    "Data/Textures/char_cinnabar.png",
    "Data/Textures/Entities/char_cinnabar_full.png",
    "Data/Textures/char_cyan.DDS",
    "Data/Textures/char_cyan.png",
    "Data/Textures/Entities/char_cyan_full.png",
    "Data/Textures/char_gold.DDS",
    "Data/Textures/char_gold.png",
    "Data/Textures/Entities/char_gold_full.png",
    "Data/Textures/char_gray.DDS",
    "Data/Textures/char_gray.png",
    "Data/Textures/Entities/char_gray_full.png",
    "Data/Textures/char_green.DDS",
    "Data/Textures/char_green.png",
    "Data/Textures/Entities/char_green_full.png",
    "Data/Textures/char_iris.DDS",
    "Data/Textures/char_iris.png",
    "Data/Textures/Entities/char_iris_full.png",
    "Data/Textures/char_khaki.DDS",
    "Data/Textures/char_khaki.png",
    "Data/Textures/Entities/char_khaki_full.png",
    "Data/Textures/char_lemon.DDS",
    "Data/Textures/char_lemon.png",
    "Data/Textures/Entities/char_lemon_full.png",
    "Data/Textures/char_lime.DDS",
    "Data/Textures/char_lime.png",
    "Data/Textures/Entities/char_lime_full.png",
    "Data/Textures/char_magenta.DDS",
    "Data/Textures/char_magenta.png",
    "Data/Textures/Entities/char_magenta_full.png",
    "Data/Textures/char_olive.DDS",
    "Data/Textures/char_olive.png",
    "Data/Textures/Entities/char_olive_full.png",
};

ModManager::ModManager(std::string_view mods_root, const PlaylunkySettings& settings, VirtualFilesystem& vfs)
    : mDeveloperMode{ settings.GetBool("settings", "enable_developer_mode", false) || settings.GetBool("script_settings", "enable_developer_mode", false) }
{
    namespace fs = std::filesystem;

    LogInfo("Initializing Mod Manager...");

    LogInfo("Scanning for mods...");

    SetWriteLoadOptimization(false);

    const bool disable_asset_caching = settings.GetBool("general_settings", "disable_asset_caching", false);

    const bool speedrun_mode = settings.GetBool("general_settings", "speedrun_mode", false);
    if (speedrun_mode)
    {
        vfs.RestrictFiles({ std::begin(s_SpeedrunFiles), std::end(s_SpeedrunFiles) });
    }

    const bool enable_loose_audio_files = !speedrun_mode && (settings.GetBool("settings", "enable_loose_audio_files", false) || settings.GetBool("audio_settings", "enable_loose_audio_files", true));
    const bool cache_decoded_audio_files = enable_loose_audio_files && (settings.GetBool("settings", "cache_decoded_audio_files", false) || settings.GetBool("audio_settings", "cache_decoded_audio_files", false));
    bool load_order_updated{ false };

    const fs::path mods_root_path{ mods_root };
    if (fs::exists(mods_root_path) && fs::is_directory(mods_root_path))
    {
        const auto db_folder = mods_root_path / ".db";

        bool speedrun_mode_changed{ false };
        bool journal_gen_settings_change{ false };
        bool sticker_gen_settings_change{ false };

        {
            bool has_loose_files{ false };

            ModDatabase mod_db{ db_folder, mods_root, static_cast<ModDatabaseFlags>(ModDatabaseFlags_Files | ModDatabaseFlags_Folders) };

            const bool journal_gen = settings.GetBool("sprite_settings", "generate_character_journal_entries", true);
            const bool sticker_gen = settings.GetBool("sprite_settings", "generate_character_journal_stickers", true);
            const bool sticker_pixel_gen = settings.GetBool("sprite_settings", "generate_sticker_pixel_art", true);

            journal_gen_settings_change = mod_db.GetAdditionalSetting("generate_character_journal_entries", true) != journal_gen;
            sticker_gen_settings_change = mod_db.GetAdditionalSetting("generate_character_journal_stickers", true) != sticker_gen;
            sticker_gen_settings_change = sticker_gen_settings_change || (mod_db.GetAdditionalSetting("generate_sticker_pixel_art", true) != sticker_pixel_gen);

            speedrun_mode_changed = mod_db.GetAdditionalSetting("speedrun_mode", false) != speedrun_mode;

            mod_db.SetEnabled(true);
            mod_db.SetAdditionalSetting("speedrun_mode", speedrun_mode);
            mod_db.SetAdditionalSetting("generate_character_journal_entries", journal_gen);
            mod_db.SetAdditionalSetting("generate_character_journal_stickers", sticker_gen);
            mod_db.SetAdditionalSetting("generate_sticker_pixel_art", sticker_pixel_gen);

            mod_db.UpdateDatabase();
            mod_db.ForEachFile([&mods_root_path, &has_loose_files, &load_order_updated](const fs::path& rel_file_path, bool outdated, bool deleted, [[maybe_unused]] std::optional<bool> new_enabled_state)
                               {
                                   if (outdated)
                                   {
                                       if (algo::is_same_path(rel_file_path.extension(), ".zip"))
                                       {
                                           const fs::path zip_path = mods_root_path / rel_file_path;
                                           const auto message = fmt::format("Found archive '{}' in mods packs folder. Do you want to unzip it in order for it to be loadable as a mod?", zip_path.filename().string());
                                           if (MessageBox(NULL, message.c_str(), "Zipped Mod Found", MB_YESNO) == IDYES)
                                           {
                                               UnzipMod(zip_path);
                                           }
                                       }
                                       else if (algo::is_same_path(rel_file_path.filename(), "load_order.txt") && (outdated || deleted))
                                       {
                                           load_order_updated = true;
                                       }
                                       else
                                       {
                                           has_loose_files = true;
                                       }
                                   }
                               });

            mod_db.UpdateDatabase();
            mod_db.ForEachFolder([&mods_root_path](const fs::path& rel_folder_path, [[maybe_unused]] bool outdated, [[maybe_unused]] bool deleted, [[maybe_unused]] std::optional<bool> new_enabled_state)
                                 {
                                     const fs::path folder_path = mods_root_path / rel_folder_path;
                                     if (fs::exists(folder_path))
                                     {
                                         FixModFolderStructure(folder_path);
                                     }
                                 });

            mod_db.WriteDatabase();

            if (has_loose_files && settings.GetBool("general_settings", "enable_loose_file_warning", true))
            {
                const fs::path absolute_mods_root_path{ fs::absolute(mods_root_path) };
                LogFatal("The 'Mods/Packs' folder contains loose files, did you mean to create a subfolder to paste those files into?");
            }
        }

        const auto db_original_folder = db_folder / "Original";
        {
            const auto files = std::array{
                fs::path{ "Data/Textures/char_black.DDS" },
                fs::path{ "Data/Textures/char_blue.DDS" },
                fs::path{ "Data/Textures/char_cerulean.DDS" },
                fs::path{ "Data/Textures/char_cinnabar.DDS" },
                fs::path{ "Data/Textures/char_cyan.DDS" },
                fs::path{ "Data/Textures/char_eggchild.DDS" },
                fs::path{ "Data/Textures/char_gold.DDS" },
                fs::path{ "Data/Textures/char_gray.DDS" },
                fs::path{ "Data/Textures/char_green.DDS" },
                fs::path{ "Data/Textures/char_hired.DDS" },
                fs::path{ "Data/Textures/char_iris.DDS" },
                fs::path{ "Data/Textures/char_khaki.DDS" },
                fs::path{ "Data/Textures/char_lemon.DDS" },
                fs::path{ "Data/Textures/char_lime.DDS" },
                fs::path{ "Data/Textures/char_magenta.DDS" },
                fs::path{ "Data/Textures/char_olive.DDS" },
                fs::path{ "Data/Textures/char_orange.DDS" },
                fs::path{ "Data/Textures/char_pink.DDS" },
                fs::path{ "Data/Textures/char_red.DDS" },
                fs::path{ "Data/Textures/char_violet.DDS" },
                fs::path{ "Data/Textures/char_white.DDS" },
                fs::path{ "Data/Textures/char_yellow.DDS" },
                fs::path{ "Data/Textures/items.DDS" },
                fs::path{ "Data/Textures/mounts.DDS" },
                fs::path{ "Data/Textures/monsters_pets.DDS" },
                fs::path{ "Data/Textures/monstersbasic01.DDS" },
                fs::path{ "Data/Textures/monstersbasic02.DDS" },
                fs::path{ "Data/Textures/monstersbasic03.DDS" },
                fs::path{ "Data/Textures/monsters01.DDS" },
                fs::path{ "Data/Textures/monsters02.DDS" },
                fs::path{ "Data/Textures/monsters03.DDS" },
                fs::path{ "Data/Textures/monstersbig01.DDS" },
                fs::path{ "Data/Textures/monstersbig02.DDS" },
                fs::path{ "Data/Textures/monstersbig03.DDS" },
                fs::path{ "Data/Textures/monstersbig04.DDS" },
                fs::path{ "Data/Textures/monstersbig05.DDS" },
                fs::path{ "Data/Textures/monstersbig06.DDS" },
                fs::path{ "Data/Textures/monsters_ghost.DDS" },
                fs::path{ "Data/Textures/monsters_osiris.DDS" },
                fs::path{ "Data/Textures/journal_stickers.DDS" },
                fs::path{ "Data/Textures/journal_entry_items.DDS" },
                fs::path{ "Data/Textures/journal_entry_mons.DDS" },
                fs::path{ "Data/Textures/journal_entry_mons_big.DDS" },
                fs::path{ "Data/Textures/journal_entry_people.DDS" },
                fs::path{ "Data/Textures/menu_basic.DDS" },
                fs::path{ "Data/Textures/menu_leader.DDS" },
                fs::path{ "shaders.hlsl" },
                fs::path{ "strings00.str" },
                fs::path{ "strings01.str" },
                fs::path{ "strings02.str" },
                fs::path{ "strings03.str" },
                fs::path{ "strings04.str" },
                fs::path{ "strings05.str" },
                fs::path{ "strings06.str" },
                fs::path{ "strings07.str" },
            };
            if (ExtractGameAssets(files, db_original_folder))
            {
                LogInfo("Successfully extracted all required game assets...");

                {
                    const auto hashed_strings_file = db_original_folder / "strings_hashes.hash";
                    if (!fs::exists(hashed_strings_file))
                    {
                        if (CreateHashedStringsFile(db_original_folder / "strings00.str", hashed_strings_file))
                        {
                            LogInfo("Successfully created hashed strings file...");
                        }
                        else
                        {
                            LogError("Failed creating hashed strings file...");
                        }
                    }
                }
            }
            else
            {
                LogError("Failed extracting all required game assets, some features might not function...");
            }
        }

        const std::vector<fs::path> mod_folders = [this, mods_root_path, db_folder](const fs::path& root_folder)
        {
            std::vector<fs::path> mod_folders;

            for (fs::path sub_path : fs::directory_iterator{ root_folder })
            {
                if (fs::is_directory(sub_path) && sub_path.stem() != ".db")
                {
                    mod_folders.push_back(sub_path);
                }
            }

            // Add all mods that were deleted since last load
            {
                const auto mod_db_folder{ db_folder / "Mods" };
                if (fs::exists(mod_db_folder))
                {
                    for (fs::path sub_path : fs::directory_iterator{ mod_db_folder })
                    {
                        const auto mod_folder = mods_root_path / fs::relative(sub_path, mod_db_folder);
                        if (!algo::contains(mod_folders, mod_folder))
                        {
                            mod_folders.push_back(mod_folder);
                        }
                    }
                }
            }

            return mod_folders;
        }(mods_root_path);

        struct ModPrioAndState
        {
            std::int64_t Prio;
            bool Enabled;
        };
        std::unordered_map<std::string, ModPrioAndState> mod_name_to_prio = [&mods_root_path]()
        {
            std::unordered_map<std::string, ModPrioAndState> mod_name_to_prio;

            if (auto load_order_file = std::ifstream{ mods_root_path / "load_order.txt" })
            {
                while (!load_order_file.eof())
                {
                    std::string mod_name;
                    std::getline(load_order_file, mod_name, '\n');

                    if (!mod_name.empty())
                    {
                        bool enabled{ true };
                        if (mod_name.find("--") == 0)
                        {
                            mod_name = mod_name.substr(2);
                            mod_name = algo::trim(mod_name);
                            enabled = false;
                        }
                        mod_name_to_prio[std::move(mod_name)] = { static_cast<std::int64_t>(mod_name_to_prio.size()), enabled };
                    }
                }
            }

            return mod_name_to_prio;
        }();

        SpriteSheetMerger sprite_sheet_merger{ settings };
        sprite_sheet_merger.GatherSheetData(journal_gen_settings_change, sticker_gen_settings_change);
        StringMerger string_merger;
        bool has_outdated_shaders{ false };

        for (const fs::path& mod_folder : mod_folders)
        {
            const std::string mod_name = mod_folder.filename().string();
            const auto mod_db_folder = db_folder / "Mods" / mod_name;

            const auto [prio, enabled] = [&mod_name_to_prio, &mod_name, &mod_folder]() mutable
            {
                std::int64_t prio{ static_cast<std::int64_t>(mod_name_to_prio.size()) };
                bool enabled{ true };
                if (mod_name_to_prio.contains(mod_name))
                {
                    const auto& prio_and_state = mod_name_to_prio[mod_name];
                    prio = prio_and_state.Prio;
                    enabled = prio_and_state.Enabled;
                }
                else
                {
                    mod_name_to_prio[mod_name] = { prio, fs::exists(mod_folder) };
                }
                return std::pair{ prio, enabled };
            }();

            ModInfo mod_info{ mod_name };

            {
                ModDatabase mod_db{ mod_db_folder, mod_folder, static_cast<ModDatabaseFlags>(ModDatabaseFlags_Files | ModDatabaseFlags_Recurse) };
                mod_db.SetEnabled(enabled);

                if (mod_db.IsEnabled() || mod_db.WasEnabled())
                {
                    mod_db.UpdateDatabase();
                    mod_db.ForEachFile([&](const fs::path& rel_asset_path, bool, bool, std::optional<bool>)
                                       {
                                           if (!mod_info.HasExtendedInfo() && algo::is_same_path(rel_asset_path.filename(), "mod_info.json"))
                                           {
                                               const auto full_asset_path = mod_folder / rel_asset_path;
                                               const auto full_asset_path_string = full_asset_path.string();
                                               mod_info.ReadExtendedInfoFromJson(full_asset_path_string);
                                               mod_info.ReadFromDatabase(mod_db);
                                               mod_db.SetInfo(mod_info.Dump());
                                               sprite_sheet_merger.RegisterCustomImages(mod_folder, db_original_folder, prio, mod_info.GetCustomImages());
                                           }
                                       });
                    mod_db.ForEachFile([&](const fs::path& rel_asset_path, bool outdated, bool deleted, std::optional<bool> new_enabled_state)
                                       {
                                           const auto rel_asset_path_string = algo::path_string(rel_asset_path);

                                           const auto full_asset_path = mod_folder / rel_asset_path;
                                           const auto full_asset_path_string = algo::path_string(full_asset_path);

                                           if (disable_asset_caching)
                                           {
                                               outdated = !deleted;
                                           }

                                           if (speedrun_mode_changed)
                                           {
                                               outdated = true;
                                               deleted = true;
                                           }

                                           if (algo::is_same_path(rel_asset_path.extension(), ".lvl"))
                                           {
                                               Playlunky::Get().RegisterModType(ModType::Level);
                                           }
                                           else if (algo::is_same_path(rel_asset_path.extension(), ".dds"))
                                           {
                                               const bool is_character_asset = ctre::match<s_CharacterRule>(full_asset_path_string);
                                               Playlunky::Get().RegisterModType(is_character_asset ? ModType::CharacterSprite : ModType::Sprite);
                                           }
                                           else if (algo::is_same_path(rel_asset_path.extension(), ".png"))
                                           {
                                               const bool is_entity_asset = algo::contains_if(rel_asset_path,
                                                                                              [](const fs::path& element)
                                                                                              { return algo::is_same_path(element, "Entities"); });
                                               const bool is_character_asset = ctre::match<s_CharacterRule>(full_asset_path_string);
                                               const bool is_custom_image_source = mod_info.IsCustomImageSource(rel_asset_path_string);

                                               Playlunky::Get().RegisterModType(is_character_asset ? ModType::CharacterSprite : ModType::Sprite);

                                               if (is_entity_asset || is_character_asset || is_custom_image_source)
                                               {
                                                   sprite_sheet_merger.RegisterSheet(rel_asset_path, outdated || load_order_updated, deleted);
                                                   return;
                                               }

                                               if (outdated || deleted)
                                               {
                                                   const auto db_destination = (mod_db_folder / rel_asset_path).replace_extension(".dds");

                                                   if (deleted)
                                                   {
                                                       if (fs::remove(db_destination))
                                                       {
                                                           LogInfo("Successfully deleted file '{}' that was removed from a mod...", full_asset_path.string());
                                                       }
                                                   }
                                                   else if (ConvertPngToDds(full_asset_path, db_destination))
                                                   {
                                                       LogInfo("Successfully converted file '{}' to be readable by the game...", full_asset_path.string());
                                                   }
                                                   else
                                                   {
                                                       LogError("Failed converting file '{}' to be readable by the game...", full_asset_path.string());
                                                   }
                                               }
                                           }
                                           else if (algo::is_same_path(rel_asset_path.extension(), ".str"))
                                           {
                                               if (outdated || deleted || new_enabled_state.has_value())
                                               {
                                                   if (auto string_match = ctre::match<s_StringFileRule>(rel_asset_path_string))
                                                   {
                                                       const auto table = string_match.get<1>().to_view();
                                                       if (!string_merger.RegisterOutdatedStringTable(table))
                                                       {
                                                           LogInfo("String file {} is not a valid string file...", full_asset_path_string);
                                                       }
                                                   }
                                                   else if (auto string_mod_match = ctre::match<s_StringModFileRule>(rel_asset_path_string))
                                                   {
                                                       const auto table = string_mod_match.get<1>().to_view();
                                                       if (!string_merger.RegisterOutdatedStringTable(table) || !string_merger.RegisterModdedStringTable(table))
                                                       {
                                                           LogInfo("String mod {} is not a valid string mod...", full_asset_path_string);
                                                       }
                                                   }
                                               }
                                           }
                                           else if (algo::is_same_path(rel_asset_path, "shaders_mod.hlsl"))
                                           {
                                               has_outdated_shaders = has_outdated_shaders || outdated || deleted || new_enabled_state.has_value();
                                           }
                                           else if (cache_decoded_audio_files && IsSupportedAudioFile(rel_asset_path))
                                           {
                                               if (deleted)
                                               {
                                                   DeleteCachedAudioFile(full_asset_path, mod_db_folder);
                                               }
                                               else if (!HasCachedAudioFile(full_asset_path, mod_db_folder))
                                               {
                                                   if (CacheAudioFile(full_asset_path, mod_db_folder, outdated))
                                                   {
                                                       LogInfo("Successfully cached audio file '{}'...", full_asset_path.string());
                                                   }
                                                   else
                                                   {
                                                       LogError("Failed caching audio file '{}'...", full_asset_path.string());
                                                   }
                                               }
                                           }
                                           else if (!speedrun_mode && enabled && !deleted && algo::is_same_path(rel_asset_path.filename(), "main.lua"))
                                           {
                                               if (mScriptManager.RegisterModWithScript(mod_name, full_asset_path, prio, enabled))
                                               {
                                                   LogInfo("Mod {} registered as a script mod with entry {}...", mod_name, full_asset_path_string);
                                               }
                                               else
                                               {
                                                   LogError("Mod {} appears to contain multiple main.lua files... {} will be ignored...", mod_name, full_asset_path_string);
                                               }
                                           }
                                       });
                    mod_db.WriteDatabase();
                }
            }

            if (fs::exists(mod_folder))
            {
                if (enabled)
                {
                    vfs.MountFolder(mod_folder.string(), prio);
                    vfs.MountFolder(mod_db_folder.string(), prio);
                }

                mMods.push_back(std::move(mod_info));
            }
        }

        // Bind char pathes
        vfs.BindPathes({ "Data/Textures/char_orange.png", "Data/Textures/Entities/char_orange_full.png" });
        vfs.BindPathes({ "Data/Textures/char_pink.png", "Data/Textures/Entities/char_pink_full.png" });
        vfs.BindPathes({ "Data/Textures/char_red.png", "Data/Textures/Entities/char_red_full.png" });
        vfs.BindPathes({ "Data/Textures/char_violet.png", "Data/Textures/Entities/char_violet_full.png" });
        vfs.BindPathes({ "Data/Textures/char_white.png", "Data/Textures/Entities/char_white_full.png" });
        vfs.BindPathes({ "Data/Textures/char_yellow.png", "Data/Textures/Entities/char_yellow_full.png" });
        vfs.BindPathes({ "Data/Textures/char_black.png", "Data/Textures/Entities/char_black_full.png" });
        vfs.BindPathes({ "Data/Textures/char_blue.png", "Data/Textures/Entities/char_blue_full.png" });
        vfs.BindPathes({ "Data/Textures/char_cerulean.png", "Data/Textures/Entities/char_cerulean_full.png" });
        vfs.BindPathes({ "Data/Textures/char_cinnabar.png", "Data/Textures/Entities/char_cinnabar_full.png" });
        vfs.BindPathes({ "Data/Textures/char_cyan.png", "Data/Textures/Entities/char_cyan_full.png" });
        vfs.BindPathes({ "Data/Textures/char_eggchild.png", "Data/Textures/Entities/char_eggchild_full.png" });
        vfs.BindPathes({ "Data/Textures/char_gold.png", "Data/Textures/Entities/char_gold_full.png" });
        vfs.BindPathes({ "Data/Textures/char_gray.png", "Data/Textures/Entities/char_gray_full.png" });
        vfs.BindPathes({ "Data/Textures/char_green.png", "Data/Textures/Entities/char_green_full.png" });
        vfs.BindPathes({ "Data/Textures/char_hired.png", "Data/Textures/Entities/char_hired_full.png" });
        vfs.BindPathes({ "Data/Textures/char_iris.png", "Data/Textures/Entities/char_iris_full.png" });
        vfs.BindPathes({ "Data/Textures/char_khaki.png", "Data/Textures/Entities/char_khaki_full.png" });
        vfs.BindPathes({ "Data/Textures/char_lemon.png", "Data/Textures/Entities/char_lemon_full.png" });
        vfs.BindPathes({ "Data/Textures/char_lime.png", "Data/Textures/Entities/char_lime_full.png" });
        vfs.BindPathes({ "Data/Textures/char_magenta.png", "Data/Textures/Entities/char_magenta_full.png" });
        vfs.BindPathes({ "Data/Textures/char_olive.png", "Data/Textures/Entities/char_olive_full.png" });

        vfs.BindPathes({ "Data/Textures/Entities/monty_full.png", "Data/Textures/Entities/Pets/monty.png", "Data/Textures/Entities/Pets/monty_v2.png" });
        vfs.BindPathes({ "Data/Textures/Entities/percy_full.png", "Data/Textures/Entities/Pets/percy.png", "Data/Textures/Entities/Pets/percy_v2.png" });
        vfs.BindPathes({ "Data/Textures/Entities/poochi_full.png", "Data/Textures/Entities/Pets/poochi.png", "Data/Textures/Entities/Pets/poochi_v2.png" });
        vfs.BindPathes({ "Data/Textures/Entities/turkey_full.png", "Data/Textures/Entities/Mounts/turkey.png" });
        vfs.BindPathes({ "Data/Textures/Entities/rockdog_full.png", "Data/Textures/Entities/Mounts/rockdog.png" });
        vfs.BindPathes({ "Data/Textures/Entities/axolotl_full.png", "Data/Textures/Entities/Mounts/axolotl.png" });
        vfs.BindPathes({ "Data/Textures/Entities/qilin_full.png", "Data/Textures/Entities/Mounts/qilin.png" });

        LogInfo("Merging entity sheets... This includes the automatic generating of stickers...");
        if (sprite_sheet_merger.NeedsRegeneration(db_folder))
        {
            if (sprite_sheet_merger.GenerateRequiredSheets(db_original_folder, db_folder, vfs))
            {
                LogInfo("Successfully generated merged sheets from mods...");
            }
            else
            {
                LogError("Failed generating merged sheets from mods...");
            }
        }

        LogInfo("Merging shader mods...");
        if (has_outdated_shaders || !fs::exists(db_folder / "shaders.hlsl"))
        {
            if (MergeShaders(db_original_folder, db_folder, "shaders.hlsl", vfs))
            {
                LogInfo("Successfully generated a full shader file from installed shader mods...");
            }
            else
            {
                LogError("Failed generating a full shader file from installed shader mods...");
            }
        }

        LogInfo("Merging string mods...");
        if (string_merger.NeedsRegen() || !fs::exists(db_folder / "strings00.str"))
        {
            if (string_merger.MergeStrings(db_original_folder, db_folder, "strings_hashes.hash", vfs))
            {
                LogInfo("Successfully generated a full string file from installed string mods...");
            }
            else
            {
                LogError("Failed generating a full string file from installed string mods...");
            }
        }
        PatchCharacterDefinitions(vfs, settings);

        vfs.MountFolder(db_folder.string(), -1);

        {
            struct ModNameAndState
            {
                std::string Name;
                bool Enabled;
            };
            std::map<std::int64_t, ModNameAndState> mod_prio_to_name;
            for (auto& [mod_name, prio_and_state] : mod_name_to_prio)
            {
                mod_prio_to_name[prio_and_state.Prio] = { mod_name, prio_and_state.Enabled };
            }

            if (auto load_order_file = std::ofstream{ mods_root_path / "load_order.txt", std::ios::trunc })
            {
                for (auto& [prio, mod_name_and_state] : mod_prio_to_name)
                {
                    if (algo::contains(mMods, &ModInfo::GetNameInternal, mod_name_and_state.Name))
                    {
                        if (!mod_name_and_state.Enabled)
                        {
                            load_order_file << "--";
                        }
                        load_order_file << mod_name_and_state.Name << '\n';
                    }
                }
            }
        }

        {
            // Rewrite mod database so we don't trigger changes on files written during mod load (e.g. load_order.txt)
            ModDatabase mod_db{ db_folder, mods_root, static_cast<ModDatabaseFlags>(ModDatabaseFlags_Files | ModDatabaseFlags_Folders) };
            mod_db.UpdateDatabase();
            mod_db.WriteDatabase();
        }

        if (Playlunky::Get().IsModTypeLoaded(ModType::Script | ModType::Level))
        {
            SetWriteLoadOptimization(true);
        }

        LogInfo("All mods initialized...");
    }
    else
    {
        const fs::path absolute_mods_root_path{ fs::absolute(mods_root_path) };
        LogFatal("The 'Mods/Packs' folder does not exist, did you do a mistake when installing mods or did you not mean to run Playlunky? "
                 "The folder was expected to be found here: {}",
                 absolute_mods_root_path.string());

        LogInfo("No mods were initialized...");
    }
}
ModManager::~ModManager() = default;

void ModManager::PostGameInit()
{
    InitSoundManager([](const char* file_path)
                     {
                         DecodedAudioBuffer buffer = DecodeAudioFile(std::filesystem::path{ file_path });
                         return Spelunky_DecodedAudioBuffer{
                             .num_channels{ buffer.NumChannels },
                             .frequency{ buffer.Frequency },
                             .format{ static_cast<Spelunky_SoundFormat>(buffer.Format) },
                             .data{ reinterpret_cast<const char*>(buffer.Data.release()) },
                             .data_size{ buffer.DataSize }
                         };
                     });
    mScriptManager.CommitScripts();

    RegisterOnInputFunc(FunctionPointer<std::remove_pointer_t<OnInputFunc>, struct ModManagerOnInput>(&ModManager::OnInput, this));
    RegisterPreDrawFunc(FunctionPointer<std::remove_pointer_t<PreDrawFunc>, struct ModManagerUpdate>(&ModManager::Update, this));
    RegisterImguiDrawFunc(FunctionPointer<std::remove_pointer_t<ImguiDrawFunc>, struct ModManagerDraw>(&ModManager::Draw, this));

    RegisterMakeSavePathFunc([](
                                 const char* script_path, size_t script_path_size, const char* /*script_name*/, size_t /*script_name_size*/, char* out_buffer, size_t out_buffer_size) -> bool
                             {
                                 auto fmt_res = fmt::format_to_n(out_buffer, out_buffer_size - 1, "{}/save.dat", std::string_view{ script_path, script_path_size });
                                 out_buffer[fmt_res.size] = '\0';
                                 return true;
                             });
}

bool ModManager::OnInput(std::uint32_t msg, std::uint64_t w_param, std::int64_t /*l_param*/)
{
    if (msg == WM_KEYUP)
    {
        if (w_param == VK_F4)
        {
            if (GetKeyState(VK_CONTROL))
            {
                mScriptManager.ToggleForceShowOptions();
            }
        }
        else if (w_param == VK_F5)
        {
            if (GetKeyState(VK_CONTROL))
            {
                if (mDeveloperMode)
                {
                    mScriptManager.RefreshScripts();
                }
            }
        }
    }
    return false;
}
void ModManager::Update()
{
    mScriptManager.Update();
}
void ModManager::Draw()
{
    mScriptManager.Draw();
    DrawImguiOverlay();
}
