#include "mod_manager.h"

#include "bug_fixes.h"
#include "cache_audio_file.h"
#include "dds_conversion.h"
#include "decode_audio_file.h"
#include "extract_game_assets.h"
#include "fix_mod_structure.h"
#include "known_files.h"
#include "mod_database.h"
#include "mod_info.h"
#include "patch_character_definitions.h"
#include "playlunky.h"
#include "playlunky_settings.h"
#include "save_game.h"
#include "shader_merge.h"
#include "sprite_hot_loader.h"
#include "sprite_painter.h"
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

static constexpr ctll::fixed_string s_ColorTextureRule{ ".*_col\\.(dds|bmp|dib|jpeg|jpg|jpe|jp2|png|webp|pbm|pgm|ppm|sr|ras|tiff|tif)" };
static constexpr ctll::fixed_string s_LuminosityTextureRule{ ".*_lumin\\.(dds|bmp|dib|jpeg|jpg|jpe|jp2|png|webp|pbm|pgm|ppm|sr|ras|tiff|tif)" };
static constexpr ctll::fixed_string s_StringFileRule{ "strings([0-9]{2})\\.str" };
static constexpr ctll::fixed_string s_StringModFileRule{ "strings([0-9]{2})_mod\\.str" };

ModManager::ModManager(std::string_view mods_root, PlaylunkySettings& settings, VirtualFilesystem& vfs)
    : mSpriteSheetMerger{ new SpriteSheetMerger{ settings } }
    , mVfs{ vfs }
    , mModsRoot{ mods_root }
    , mDeveloperMode{ settings.GetBool("settings", "enable_developer_mode", false) || settings.GetBool("script_settings", "enable_developer_mode", false) }
    , mConsoleMode{ settings.GetBool("script_settings", "enable_developer_console", false) }
    , mConsoleKey{ static_cast<std::uint64_t>(settings.GetInt("key_bindings", "console", VK_OEM_3)) }
    , mConsoleAltKey{ static_cast<std::uint64_t>(settings.GetInt("key_bindings", "console_alt", VK_OEM_5)) }
    , mConsoleCloseKey{ static_cast<std::uint64_t>(settings.GetInt("key_bindings", "console_close", VK_ESCAPE)) }
{
    namespace fs = std::filesystem;

    LogInfo("Initializing Mod Manager...");

    LogInfo("Scanning for mods...");

    Spelunky_InitMemoryDatabase();
    Spelunky_SetWriteLoadOptimization(false);

    const bool disable_asset_caching = settings.GetBool("general_settings", "disable_asset_caching", false);

    const bool speedrun_mode = settings.GetBool("general_settings", "speedrun_mode", false);
    const bool enable_raw_string_loading = !speedrun_mode && settings.GetBool("script_settings", "enable_raw_string_loading", false);
    const bool enable_customizable_sheets = !speedrun_mode && settings.GetBool("sprite_settings", "enable_customizable_sheets", true);

    if (speedrun_mode)
    {
        vfs.RestrictFiles({ std::begin(s_SpeedrunFiles), std::end(s_SpeedrunFiles) });
    }

    const bool enable_loose_audio_files = !speedrun_mode && (settings.GetBool("settings", "enable_loose_audio_files", false) || settings.GetBool("audio_settings", "enable_loose_audio_files", true));
    const bool cache_decoded_audio_files = enable_loose_audio_files && (settings.GetBool("settings", "cache_decoded_audio_files", false) || settings.GetBool("audio_settings", "cache_decoded_audio_files", false));
    bool load_order_updated{ false };

    const fs::path mods_root_path{ mModsRoot };
    if (fs::exists(mods_root_path) && fs::is_directory(mods_root_path))
    {
        const auto db_folder{ mods_root_path / ".db" };
        const auto mod_db_folder{ db_folder / "Mods" };

        bool speedrun_mode_changed{ false };
        bool journal_gen_settings_change{ false };
        bool sticker_gen_settings_change{ false };

        {
            bool has_loose_files{ false };

            ModDatabase mod_db{ db_folder, mods_root, static_cast<ModDatabaseFlags>(ModDatabaseFlags_Files | ModDatabaseFlags_Folders) };

            if (mod_db.WasOutdated())
            {
                settings.SetBool("bug_fixes", "missing_pipes", false);
            }

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
                                   } });

            mod_db.UpdateDatabase();
            mod_db.ForEachFolder([&mods_root_path](const fs::path& rel_folder_path, [[maybe_unused]] bool outdated, [[maybe_unused]] bool deleted, [[maybe_unused]] std::optional<bool> new_enabled_state)
                                 {
                                     const fs::path folder_path = mods_root_path / rel_folder_path;
                                     if (fs::exists(folder_path))
                                     {
                                         FixModFolderStructure(folder_path);
                                     } });

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
                fs::path{ "Data/Textures/deco_cave.DDS" },
                fs::path{ "shaders.hlsl" },
                fs::path{ "strings00.str" },
                fs::path{ "strings01.str" },
                fs::path{ "strings02.str" },
                fs::path{ "strings03.str" },
                fs::path{ "strings04.str" },
                fs::path{ "strings05.str" },
                fs::path{ "strings06.str" },
                fs::path{ "strings07.str" },
                fs::path{ "strings08.str" },
                fs::path{ "strings09.str" },
                fs::path{ "strings10.str" },
                fs::path{ "strings11.str" },
                fs::path{ "strings12.str" },
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

        const std::vector<fs::path> mod_folders = [this, mods_root_path, mod_db_folder](const fs::path& root_folder)
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

        const bool enable_sprite_hot_loading = settings.GetBool("sprite_settings", "enable_sprite_hot_loading", false);
        if (enable_sprite_hot_loading)
        {
            mSpriteHotLoader = std::make_unique<SpriteHotLoader>(*mSpriteSheetMerger, settings);
        }

        mSpriteSheetMerger->GatherSheetData(journal_gen_settings_change, sticker_gen_settings_change);
        StringMerger string_merger;
        bool has_outdated_shaders{ false };

        for (const fs::path& mod_folder : mod_folders)
        {
            const std::string mod_name = mod_folder.filename().string();
            const auto this_db_folder = db_folder / "Mods" / mod_name;

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
                ModDatabase mod_db{ this_db_folder, mod_folder, static_cast<ModDatabaseFlags>(ModDatabaseFlags_Files | ModDatabaseFlags_Recurse) };
                mod_db.SetEnabled(enabled);

                if (mod_db.IsEnabled() || mod_db.WasEnabled())
                {
                    mod_db.UpdateDatabase();
                    const std::vector<fs::path> mod_load_paths{ this_db_folder, mod_folder };
                    if (mod_db.IsEnabled())
                    {
                        mod_db.ForEachFile([&](const fs::path& rel_asset_path, bool, bool, std::optional<bool>)
                                           {
                                               if (!mod_info.HasExtendedInfo() && algo::is_same_path(rel_asset_path.filename(), "mod_info.json"))
                                               {
                                                   const auto full_asset_path = mod_folder / rel_asset_path;
                                                   const auto full_asset_path_string = full_asset_path.string();
                                                   mod_info.ReadExtendedInfoFromJson(full_asset_path_string);
                                                   mod_info.ReadFromDatabase(mod_db);
                                                   mod_db.SetInfo(mod_info.Dump());
                                                   mSpriteSheetMerger->RegisterCustomImages(mod_name, mod_load_paths, db_original_folder, prio, mod_info.GetCustomImages());
                                               } });
                    }
                    else
                    {
                        mod_info.ReadFromDatabase(mod_db);
                        mod_db.SetInfo("");
                        mSpriteSheetMerger->RegisterCustomImages(mod_name, mod_load_paths, db_original_folder, prio, mod_info.GetCustomImages());
                    }
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
                                           }

                                           if (algo::is_same_path(rel_asset_path.extension(), ".lvl"))
                                           {
                                               Playlunky::Get().RegisterModType(ModType::Level);
                                           }
                                           else if (algo::is_same_path(rel_asset_path.extension(), ".dds"))
                                           {
                                               const bool is_character_asset = algo::contains(s_KnownCharFiles, rel_asset_path.stem());
                                               Playlunky::Get().RegisterModType(is_character_asset ? ModType::CharacterSprite : ModType::Sprite);
                                           }
                                           else if (IsSupportedFileType(rel_asset_path.extension()))
                                           {
                                               const auto rel_asset_file_name = rel_asset_path.filename().string();
                                               if (ctre::match<s_ColorTextureRule>(rel_asset_file_name))
                                               {
                                                   if (!mSpritePainter)
                                                   {
                                                       mSpritePainter = std::make_unique<SpritePainter>(*mSpriteSheetMerger, vfs, settings, db_original_folder);
                                                   }

                                                   if (!enable_customizable_sheets)
                                                   {
                                                       deleted = true;
                                                   }

                                                   // Does not necessarily write dds to the db
                                                   const auto db_destination = this_db_folder / rel_asset_path;
                                                   mSpritePainter->RegisterSheet(full_asset_path, db_destination, outdated, deleted);
                                                   return;
                                               }
                                               if (ctre::match<s_LuminosityTextureRule>(rel_asset_file_name))
                                               {
                                                   return;
                                               }

                                               const bool is_entity_asset = algo::contains_if(rel_asset_path,
                                                                                              [](const fs::path& element)
                                                                                              { return algo::is_same_path(element, "Entities"); });
                                               const bool is_character_asset = algo::contains(s_KnownCharFiles, rel_asset_path.stem());
                                               const bool is_custom_image_source = mod_info.IsCustomImageSource(rel_asset_path_string);

                                               Playlunky::Get().RegisterModType(is_character_asset ? ModType::CharacterSprite : ModType::Sprite);

                                               if (mSpriteHotLoader)
                                               {
                                                   const auto db_destination = (this_db_folder / rel_asset_path).replace_extension(".DDS");
                                                   mSpriteHotLoader->RegisterSheet(full_asset_path, db_destination);
                                               }

                                               if (is_entity_asset || is_character_asset || is_custom_image_source)
                                               {
                                                   mSpriteSheetMerger->RegisterSheet(rel_asset_path, outdated || load_order_updated, deleted);
                                                   return;
                                               }

                                               if (outdated || deleted)
                                               {
                                                   const auto db_destination = (this_db_folder / rel_asset_path).replace_extension(".DDS");

                                                   if (deleted)
                                                   {
                                                       if (fs::remove(db_destination))
                                                       {
                                                           LogInfo("Successfully deleted file '{}' that was removed from a mod...", full_asset_path.string());
                                                       }
                                                   }
                                                   else if (ConvertImageToDds(full_asset_path, db_destination))
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
                                                   DeleteCachedAudioFile(full_asset_path, this_db_folder);
                                               }
                                               else if (!HasCachedAudioFile(full_asset_path, this_db_folder))
                                               {
                                                   if (CacheAudioFile(full_asset_path, this_db_folder, outdated))
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
                                           } });
                    mod_db.WriteDatabase();
                }
            }

            if (fs::exists(mod_folder))
            {
                if (enabled)
                {
                    vfs.MountFolder(this_db_folder.string(), prio);
                    vfs.MountFolder(mod_folder.string(), prio, VfsType::User);
                }

                mMods.push_back(std::move(mod_info));
            }
        }

        {
            // Bind char pathes
            vfs.BindPathes({ "Data/Textures/char_orange", "Data/Textures/Entities/char_orange_full" });
            vfs.BindPathes({ "Data/Textures/char_pink", "Data/Textures/Entities/char_pink_full" });
            vfs.BindPathes({ "Data/Textures/char_red", "Data/Textures/Entities/char_red_full" });
            vfs.BindPathes({ "Data/Textures/char_violet", "Data/Textures/Entities/char_violet_full" });
            vfs.BindPathes({ "Data/Textures/char_white", "Data/Textures/Entities/char_white_full" });
            vfs.BindPathes({ "Data/Textures/char_yellow", "Data/Textures/Entities/char_yellow_full" });
            vfs.BindPathes({ "Data/Textures/char_black", "Data/Textures/Entities/char_black_full" });
            vfs.BindPathes({ "Data/Textures/char_blue", "Data/Textures/Entities/char_blue_full" });
            vfs.BindPathes({ "Data/Textures/char_cerulean", "Data/Textures/Entities/char_cerulean_full" });
            vfs.BindPathes({ "Data/Textures/char_cinnabar", "Data/Textures/Entities/char_cinnabar_full" });
            vfs.BindPathes({ "Data/Textures/char_cyan", "Data/Textures/Entities/char_cyan_full" });
            vfs.BindPathes({ "Data/Textures/char_eggchild", "Data/Textures/Entities/char_eggchild_full" });
            vfs.BindPathes({ "Data/Textures/char_gold", "Data/Textures/Entities/char_gold_full" });
            vfs.BindPathes({ "Data/Textures/char_gray", "Data/Textures/Entities/char_gray_full" });
            vfs.BindPathes({ "Data/Textures/char_green", "Data/Textures/Entities/char_green_full" });
            vfs.BindPathes({ "Data/Textures/char_hired", "Data/Textures/Entities/char_hired_full" });
            vfs.BindPathes({ "Data/Textures/char_iris", "Data/Textures/Entities/char_iris_full" });
            vfs.BindPathes({ "Data/Textures/char_khaki", "Data/Textures/Entities/char_khaki_full" });
            vfs.BindPathes({ "Data/Textures/char_lemon", "Data/Textures/Entities/char_lemon_full" });
            vfs.BindPathes({ "Data/Textures/char_lime", "Data/Textures/Entities/char_lime_full" });
            vfs.BindPathes({ "Data/Textures/char_magenta", "Data/Textures/Entities/char_magenta_full" });
            vfs.BindPathes({ "Data/Textures/char_olive", "Data/Textures/Entities/char_olive_full" });

            vfs.BindPathes({ "Data/Textures/Entities/monty_full", "Data/Textures/Entities/Pets/monty", "Data/Textures/Entities/Pets/monty_v2" });
            vfs.BindPathes({ "Data/Textures/Entities/percy_full", "Data/Textures/Entities/Pets/percy", "Data/Textures/Entities/Pets/percy_v2" });
            vfs.BindPathes({ "Data/Textures/Entities/poochi_full", "Data/Textures/Entities/Pets/poochi", "Data/Textures/Entities/Pets/poochi_v2" });
            vfs.BindPathes({ "Data/Textures/Entities/turkey_full", "Data/Textures/Entities/Mounts/turkey" });
            vfs.BindPathes({ "Data/Textures/Entities/rockdog_full", "Data/Textures/Entities/Mounts/rockdog" });
            vfs.BindPathes({ "Data/Textures/Entities/axolotl_full", "Data/Textures/Entities/Mounts/axolotl" });
            vfs.BindPathes({ "Data/Textures/Entities/qilin_full", "Data/Textures/Entities/Mounts/qilin" });
        }

        if (speedrun_mode)
        {
            vfs.RegisterCustomFilter(
                [db_folder, mod_db_folder](const fs::path& asset_path, std::string_view relative_path) -> bool
                {
                    const bool in_mod_db_folder = algo::is_sub_path(asset_path, mod_db_folder);
                    const bool in_db_folder = in_mod_db_folder || algo::is_sub_path(asset_path, db_folder);
                    if (!in_db_folder || in_mod_db_folder)
                    {
                        // Strip extension
                        {
                            const size_t ext_pos = relative_path.find(".");
                            if (ext_pos != std::string_view::npos)
                            {
                                relative_path = relative_path.substr(0, ext_pos);
                            }
                        }

                        return !algo::contains(s_SpeedrunDbFiles, relative_path);
                    }
                    return true;
                });
        }
        else if (!enable_raw_string_loading)
        {
            vfs.RegisterCustomFilter(
                [db_folder](const fs::path& asset_path, [[maybe_unused]] std::string_view relative_path) -> bool
                {
                    if (asset_path.extension() == L".str")
                    {
                        return ctre::match<s_StringModFileRule>(asset_path.filename().string()) || algo::is_sub_path(asset_path, db_folder);
                    }
                    return true;
                });
        }

        LogInfo("Merging entity sheets... This includes the automatic generating of stickers...");
        if (mSpriteSheetMerger->NeedsRegeneration(db_folder))
        {
            if (mSpriteSheetMerger->GenerateRequiredSheets(db_original_folder, db_folder, vfs))
            {
                LogInfo("Successfully generated merged sheets from mods...");
            }
            else
            {
                LogError("Failed generating merged sheets from mods...");
            }
        }

        if (enable_sprite_hot_loading && mSpriteHotLoader)
        {
            LogInfo("Setting up sprite hot-loading...");
            mSpriteHotLoader->FinalizeSetup();
        }
        else if (!mSpritePainter)
        {
            mSpriteSheetMerger.reset();
        }

        if (!enable_customizable_sheets && mSpritePainter)
        {
            mSpritePainter.reset();
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
            if (string_merger.MergeStrings(db_original_folder, db_folder, "strings_hashes.hash", speedrun_mode, vfs))
            {
                LogInfo("Successfully generated a full string file from installed string mods...");
            }
            else
            {
                LogError("Failed generating a full string file from installed string mods...");
            }
        }

        // Mounting early to maintain guarantees about VFS immutability
        BugFixesMount(vfs, db_folder);

        vfs.MountFolder(db_folder.string(), -1, VfsType::Backend);
        vfs.MountFolder("", -2, VfsType::Backend);

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
            Spelunky_SetWriteLoadOptimization(true);
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

    if (const bool disable_steam_achievements = settings.GetBool("general_settings", "disable_steam_achievements", false))
    {
        Spelunky_DisableSteamAchievements();
    }

    Spelunky_RegisterOnLoadFileFunc(FunctionPointer<Spelunky_LoadFileFunc, struct ModManagerLoadFile>(
        [this](const char* file_path, SpelunkyAllocFun alloc_fun) -> SpelunkyFileInfo*
        {
            if (auto* file_info = mVfs.LoadFile(file_path, alloc_fun))
            {
                return file_info;
            }
            return nullptr;
        }));
    Spelunky_RegisterGetImagePathFunc(FunctionPointer<Spelunky_GetImageFilePathFunc, struct ModManagerGetImagePath>(
        [this](const char* root_path, const char* relative_path, char* out_buffer, size_t out_buffer_size) -> bool
        {
            auto dds_relative_path = std::filesystem::path(relative_path).replace_extension(".DDS");
            auto dds_relative_path_str = dds_relative_path.string();
            if (algo::contains(s_KnownTextureFiles, dds_relative_path.stem().string()))
            {
                auto fmt_res = fmt::format_to_n(
                    out_buffer,
                    out_buffer_size - 1,
                    "{}",
                    dds_relative_path_str);
                out_buffer[fmt_res.size] = '\0';
                return fmt_res.size < out_buffer_size;
            }

            auto [mod_name, mod_rel_path] = [](std::filesystem::path root)
            {
                std::string mod_name;
                std::filesystem::path mod_rel_path;
                bool found_packs{ false };

                for (auto seg : root)
                {
                    if (found_packs)
                    {
                        if (mod_name.empty())
                        {
                            mod_name = seg.string();
                        }
                        else
                        {
                            mod_rel_path /= seg;
                        }
                    }
                    else if (algo::is_same_path(seg, "Packs"))
                    {
                        found_packs = true;
                    }
                }

                return std::pair{ std::move(mod_name), mod_rel_path.string() };
            }(root_path);

            auto fmt_res = fmt::format_to_n(
                out_buffer,
                out_buffer_size - 1,
                "Mods/Packs/.db/Mods/{}{}{}/{}",
                mod_name,
                mod_rel_path.empty() ? "" : "/",
                mod_rel_path,
                dds_relative_path_str);
            out_buffer[fmt_res.size] = '\0';
            return fmt_res.size < out_buffer_size;
        }));

    const SaveGameMod save_mod_type = [](const PlaylunkySettings& settings)
    {
        if (const bool block_save_game = settings.GetBool("general_settings", "block_save_game", false))
        {
            return SaveGameMod::Block;
        }
        else
        {
            const bool allow_save_game_mods = settings.GetBool("general_settings", "allow_save_game_mods", true);
            if (const bool use_playlunky_save = settings.GetBool("general_settings", "use_playlunky_save", false))
            {
                return allow_save_game_mods ? SaveGameMod::SeparateSaveOrFromMod : SaveGameMod::SeparateSave;
            }
            else if (allow_save_game_mods)
            {
                return SaveGameMod::FromMod;
            }
        }
        return SaveGameMod::None;
    }(settings);
    if (save_mod_type != SaveGameMod::None)
    {
        using namespace std::string_view_literals;
        if (save_mod_type != SaveGameMod::Block)
        {
            // Prepare for warning
            if (auto sav_replacement = vfs.GetDifferentFilePath("savegame.sav"))
            {
                mModSaveGameOverride = sav_replacement.value().parent_path().stem().string();
            }

            Spelunky_RegisterOnReadFromFileFunc(FunctionPointer<Spelunky_ReadFromFileFunc, struct ModManagerSaveFile>(
                [=](const char* file, void** out_data, size_t* out_data_size, [[maybe_unused]] SpelunkyAllocFun alloc_fun, Spelunky_ReadFromFileOriginal original)
                {
                    if (file == "savegame.sav"sv)
                    {
                        OnReadFromFile(save_mod_type, file, out_data, out_data_size, original);
                        return;
                    }
                    original(file, out_data, out_data_size);
                }));
        }
        Spelunky_RegisterOnWriteToFileFunc(FunctionPointer<Spelunky_WriteToFileFunc, struct ModManagerSaveFile>(
            [=](const char* backup_file, const char* file, void* data, size_t data_size, Spelunky_WriteToFileOriginal original)
            {
                if (file == "savegame.sav"sv)
                {
                    OnWriteToFile(save_mod_type, backup_file, file, data, data_size, original);
                    return;
                }
                original(backup_file, file, data, data_size);
            }));
    }
}
ModManager::~ModManager()
{
    BugFixesCleanup();

    Spelunky_DestroySoundManager();

    Spelunky_RegisterOnLoadFileFunc(nullptr);
    Spelunky_RegisterGetImagePathFunc(nullptr);

    Spelunky_RegisterOnReadFromFileFunc(nullptr);
    Spelunky_RegisterOnWriteToFileFunc(nullptr);

    Spelunky_RegisterOnInputFunc(nullptr);
    Spelunky_RegisterPreDrawFunc(nullptr);
    Spelunky_RegisterImguiDrawFunc(nullptr);
    Spelunky_RegisterMakeSavePathFunc(nullptr);
}

void ModManager::PostGameInit(const class PlaylunkySettings& settings)
{
    const auto db_folder = mModsRoot / ".db";
    const auto db_original_folder = db_folder / "Original";

    if (mSpritePainter)
    {
        LogInfo("Setting up sprite painting...");
        mSpritePainter->FinalizeSetup(db_original_folder, db_folder);
    }

    PatchCharacterDefinitions(mVfs, settings);

    // Sound manager has to be initialized before any scripts
    Spelunky_InitSoundManager([](const char* file_path)
                              {
                                  DecodedAudioBuffer buffer = DecodeAudioFile(std::filesystem::path{ file_path });
                                  return Spelunky_DecodedAudioBuffer{
                                      .num_channels{ buffer.NumChannels },
                                      .frequency{ buffer.Frequency },
                                      .format{ static_cast<Spelunky_SoundFormat>(buffer.Format) },
                                      .data{ reinterpret_cast<const char*>(buffer.Data.release()) },
                                      .data_size{ buffer.DataSize }
                                  }; });

    const bool speedrun_mode = settings.GetBool("general_settings", "speedrun_mode", false);
    if (!speedrun_mode)
    {
        // Bugfixes may use scripts for some functionality
        BugFixesInit(settings, db_folder, db_original_folder);
    }

    mScriptManager.CommitScripts(settings);

    Spelunky_RegisterOnInputFunc(FunctionPointer<OnInputFunc, struct ModManagerOnInput>(&ModManager::OnInput, this));
    Spelunky_RegisterPreDrawFunc(FunctionPointer<PreDrawFunc, struct ModManagerUpdate>(&ModManager::Update, this));
    Spelunky_RegisterImguiDrawFunc(FunctionPointer<ImguiDrawFunc, struct ModManagerDraw>(&ModManager::Draw, this));

    Spelunky_RegisterMakeSavePathFunc([](
                                          const char* script_path, size_t script_path_size, const char* /*script_name*/, size_t /*script_name_size*/, char* out_buffer, size_t out_buffer_size) -> bool
                                      {
                                          auto fmt_res = fmt::format_to_n(out_buffer, out_buffer_size - 1, "{}/save.dat", std::string_view{ script_path, script_path_size });
                                          out_buffer[fmt_res.size] = '\0';
                                          return true; });
}

bool ModManager::OnInput(std::uint32_t msg, std::uint64_t w_param, std::int64_t /*l_param*/)
{
    if (msg == WM_KEYDOWN)
    {
        if (mConsoleMode)
        {
            if (w_param == mConsoleKey || w_param == mConsoleAltKey || (mScriptManager.IsConsoleToggled() && w_param == mConsoleCloseKey))
            {
                mScriptManager.ToggleConsole();
                return true;
            }
        }
    }
    else if (msg == WM_KEYUP)
    {
        if (w_param == VK_F4)
        {
            if (GetKeyState(VK_CONTROL))
            {
                mForceShowOptions = !mForceShowOptions;
            }
        }
        else if (mDeveloperMode && w_param == VK_F5)
        {
            if (GetKeyState(VK_CONTROL))
            {
                mScriptManager.RefreshScripts();
            }
        }
    }
    return false;
}
void ModManager::Update()
{
    if (mSpritePainter || mSpriteHotLoader)
    {
        const auto db_folder = mModsRoot / ".db";
        const auto db_original_folder = db_folder / "Original";
        if (mSpritePainter)
        {
            mSpritePainter->Update(db_original_folder, db_folder);
        }

        if (mSpriteHotLoader)
        {
            mSpriteHotLoader->Update(db_original_folder, db_folder, mVfs);
        }
    }

    BugFixesUpdate();

    mScriptManager.Update();
}
void ModManager::Draw()
{
    const bool show_options = mForceShowOptions || SpelunkyState_GetScreen() == SpelunkyScreen::Menu;
    if (show_options && (mScriptManager.NeedsWindowDraw() || (mSpritePainter && mSpritePainter->NeedsWindowDraw())))
    {
        if (!mShowCursor)
        {
            Spelunky_ShowCursor();
            mShowCursor = true;
        }

        ImGuiIO& io = ImGui::GetIO();

        ImGui::SetNextWindowSize({ io.DisplaySize.x / 4, io.DisplaySize.y });
        ImGui::SetNextWindowPos({ io.DisplaySize.x * 3 / 4, 0 });
        ImGui::Begin(
            "Mod Options",
            nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration);
        ImGui::PushItemWidth(100.0f);

        ImGui::TextUnformatted("Mod Options");

        if (mScriptManager.NeedsWindowDraw() || show_options)
        {
            mScriptManager.WindowDraw();
        }
        if (mSpritePainter && (mSpritePainter->NeedsWindowDraw() || mForceShowOptions))
        {
            mSpritePainter->WindowDraw();
        }

        ImGui::PopItemWidth();
        ImGui::End();
    }
    else if (mShowCursor)
    {
        Spelunky_HideCursor();
        mShowCursor = false;
    }

    mScriptManager.Draw();

    DrawImguiOverlay();

    if (!mModSaveGameOverride.empty() && SpelunkyState_GetScreen() <= SpelunkyScreen::Menu)
    {
        ImGui::SetNextWindowSize({ ImGui::GetWindowSize().x, 0 });
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, 0.0f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
        ImGui::Begin(
            "Save Game Overlay",
            nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
        ImGui::TextColored(ImColor(0.3f, 0.0f, 0.0f), "Warning: savegame.sav is overriden by mod \"%s\"", mModSaveGameOverride.c_str());
        ImGui::End();
    }
}
