#include "mod_manager.h"

#include "cache_audio_file.h"
#include "character_sticker_gen.h"
#include "extract_game_assets.h"
#include "fix_mod_structure.h"
#include "mod_database.h"
#include "png_dds_conversion.h"
#include "shader_merge.h"
#include "sprite_sheet_merger.h"
#include "string_hash.h"
#include "string_merge.h"
#include "unzip_mod.h"
#include "virtual_filesystem.h"

#include "log.h"
#include "util/algorithms.h"
#include "util/regex.h"

#include <Windows.h>
#include <zip.h>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <map>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4244)
#include <INIReader.h>
#pragma warning(pop)

static constexpr ctll::fixed_string s_CharacterRule{ ".+char_(.*)\\.png" };
static constexpr ctll::fixed_string s_CharacterFullRule{ ".+char_(.*)_full\\.png" };
static constexpr ctll::fixed_string s_StringFileRule{ "strings([0-9]{2})\\.str" };
static constexpr ctll::fixed_string s_StringModFileRule{ "strings([0-9]{2})_mod\\.str" };

ModManager::ModManager(std::string_view mods_root, VirtualFilesystem& vfs) {
	namespace fs = std::filesystem;

	LogInfo("Initializing Mod Manger...");

	LogInfo("Scanning for mods...");

	INIReader playlunky_ini("playlunky.ini");
	const bool enable_loose_audio_files = playlunky_ini.GetBoolean("settings", "enable_loose_audio_files", true);
	const bool cache_decoded_audio_files = enable_loose_audio_files && playlunky_ini.GetBoolean("settings", "cache_decoded_audio_files", true);
	bool load_order_updated{ false };

	const fs::path mods_root_path{ mods_root };
	if (fs::exists(mods_root_path) && fs::is_directory(mods_root_path)) {
		{
			bool has_loose_files{ false };

			ModDatabase mod_db{ mods_root, static_cast<ModDatabaseFlags>(ModDatabaseFlags_Files | ModDatabaseFlags_Folders) };

			mod_db.SetEnabled(true);
			mod_db.UpdateDatabase();
			mod_db.ForEachFile([&mods_root_path, &has_loose_files, &load_order_updated](const fs::path& rel_file_path, bool outdated, bool deleted, [[maybe_unused]] std::optional<bool> new_enabled_state) {
				if (outdated) {
					if (rel_file_path.extension() == ".zip") {
						const fs::path zip_path = mods_root_path / rel_file_path;
						const auto message = fmt::format("Found archive '{}' in mods packs folder. Do you want to unzip it in order for it to be loadable as a mod?", zip_path.filename().string());
						if (MessageBox(NULL, message.c_str(), "Zipped Mod Found", MB_YESNO) == IDYES) {
							UnzipMod(zip_path);
						}
					}
					else if (rel_file_path.filename() == "load_order.txt" && (outdated || deleted)) {
						load_order_updated = true;
					}
					else {
						has_loose_files = true;
					}
				}
			});

			mod_db.UpdateDatabase();
			mod_db.ForEachFolder([&mods_root_path](const fs::path& rel_folder_path, [[maybe_unused]] bool outdated, [[maybe_unused]] bool deleted, [[maybe_unused]] std::optional<bool> new_enabled_state) {
				const fs::path folder_path = mods_root_path / rel_folder_path;
				if (fs::exists(folder_path)) {
					FixModFolderStructure(folder_path);
				}
			});

			mod_db.WriteDatabase();

			if (has_loose_files) {
				const fs::path absolute_mods_root_path{ fs::absolute(mods_root_path) };
				LogFatal("The 'Mods/Packs' folder contains loose files, did you mean to create a subfolder to paste those files into?");
			}
		}

		const auto db_folder = mods_root_path / ".db";
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
				fs::path{ "Data/Textures/monsters_pets.DDS" },
				fs::path{ "Data/Textures/mounts.DDS" },
				fs::path{ "Data/Textures/journal_stickers.DDS" },
				fs::path{ "Data/Textures/journal_entry_items.DDS" },
				fs::path{ "Data/Textures/journal_entry_mons.DDS" },
				fs::path{ "Data/Textures/journal_entry_people.DDS" },
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
			};
			if (ExtractGameAssets(files, db_original_folder)) {
				LogInfo("Successfully extracted all required game assets...");

				{
					const auto hashed_strings_file = db_original_folder / "strings_hashes.hash";
					if (!fs::exists(hashed_strings_file)) {
						if (CreateHashedStringsFile(db_original_folder / "strings00.str", hashed_strings_file)) {
							LogInfo("Successfully created hashed strings file...");
						}
						else {
							LogInfo("Failed creating hashed strings file...");
						}
					}
				}
			}
			else {
				LogInfo("Failed extracting all required game assets, some features might not function...");
			}
		}

		const std::vector<fs::path> mod_folders = [this](const fs::path& root_folder) {
			std::vector<fs::path> mod_folders;

			for (fs::path sub_path : fs::directory_iterator{ root_folder }) {
				const std::string folder_name = sub_path.stem().string();
				if (fs::is_directory(sub_path) && sub_path.stem() != ".db") {
					mod_folders.push_back(sub_path);
				}
			}

			return mod_folders;
		}(mods_root_path);

		struct ModPrioAndState {
			std::int64_t Prio;
			bool Enabled;
		};
		std::unordered_map<std::string, ModPrioAndState> mod_name_to_prio = [&mods_root_path]() {
			std::unordered_map<std::string, ModPrioAndState> mod_name_to_prio;

			if (auto load_order_file = std::ifstream{ mods_root_path / "load_order.txt" }) {
				while (!load_order_file.eof()) {
					std::string mod_name;
					std::getline(load_order_file, mod_name, '\n');

					if (!mod_name.empty()) {
						bool enabled{ true };
						if (mod_name.find("--") == 0) {
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

		CharacterStickerGenerator sticker_gen;
		SpriteSheetMerger sprite_sheet_merger;
		StringMerger string_merger;
		bool has_outdated_shaders{ false };

		for (const fs::path& mod_folder : mod_folders) {
			std::string mod_name = mod_folder.filename().string(); // Not const so we can move from it later
			const auto mod_db_folder = mod_folder / ".db";

			const auto [prio, enabled] = [&mod_name_to_prio, &mod_name]() mutable {
				std::int64_t prio{ static_cast<std::int64_t>(mod_name_to_prio.size()) };
				bool enabled{ true };
				if (mod_name_to_prio.contains(mod_name)) {
					const auto& prio_and_state = mod_name_to_prio[mod_name];
					prio = prio_and_state.Prio;
					enabled = prio_and_state.Enabled;
				}
				else {
					mod_name_to_prio[mod_name] = { prio, true };
				}
				return std::pair{ prio, enabled };
			}();

			{
				ModDatabase mod_db{ mod_folder, static_cast<ModDatabaseFlags>(ModDatabaseFlags_Files | ModDatabaseFlags_Recurse) };
				mod_db.SetEnabled(enabled);
				mod_db.UpdateDatabase();
				mod_db.ForEachFile([&](const fs::path& rel_asset_path, bool outdated, bool deleted, std::optional<bool> new_enabled_state) {
					const auto rel_asset_path_string = rel_asset_path.string();

					const auto full_asset_path = mod_folder / rel_asset_path;
					const auto full_asset_path_string = full_asset_path.string();
					if (rel_asset_path.extension() == ".png") {

						if (auto character_match = ctre::match<s_CharacterRule>(full_asset_path_string)) {
							const std::string_view color = character_match.get<1>().to_view();
							if (!sticker_gen.RegisterCharacter(color, outdated || load_order_updated || new_enabled_state.has_value())) {
								const std::string character_file_name = full_asset_path.filename().string();
								if (!ctre::match<s_CharacterFullRule>(full_asset_path_string)) {
									LogInfo("Mod '{}' contains an unkown character file '{}'", mod_name, character_file_name);
								}
							}
						}

						const bool is_entity_asset = rel_asset_path.parent_path().filename() == "Entities";
						if (is_entity_asset) {
							sprite_sheet_merger.RegisterSheet(rel_asset_path, outdated || load_order_updated, deleted);
						}

						if (outdated || deleted) {
							const auto db_destination = (mod_db_folder / rel_asset_path).replace_extension(".dds");

							if (deleted) {
								if (fs::remove(db_destination)) {
									LogInfo("Successfully deleted file '{}' that was removed from a mod...", full_asset_path.string());
								}
							}
							else if (!is_entity_asset && ConvertPngToDds(full_asset_path, db_destination))
							{
								LogInfo("Successfully converted file '{}' to be readable by the game...", full_asset_path.string());
							}
							else {
								LogInfo("Failed converting file '{}' to be readable by the game...", full_asset_path.string());
							}
						}
					}
					else if (rel_asset_path.extension() == ".str") {
						if (outdated || deleted || new_enabled_state.has_value()) {
							if (auto string_match = ctre::match<s_StringFileRule>(rel_asset_path_string)) {
								const auto table = string_match.get<1>().to_view();
								if (!string_merger.RegisterOutdatedStringTable(table)) {
									LogInfo("String file {} is not a valid string file...", full_asset_path_string);
								}
							}
							else if (auto string_mod_match = ctre::match<s_StringModFileRule>(rel_asset_path_string)) {
								const auto table = string_mod_match.get<1>().to_view();
								if (!string_merger.RegisterOutdatedStringTable(table) || !string_merger.RegisterModdedStringTable(table)) {
									LogInfo("String mod {} is not a valid string mod...", full_asset_path_string);
								}
							}
						} 
					}
					else if (rel_asset_path == "shaders_mod.hlsl") {
						has_outdated_shaders = has_outdated_shaders || outdated || deleted || new_enabled_state.has_value();
					}
					else if (cache_decoded_audio_files && IsSupportedAudioFile(rel_asset_path)) {
						if (deleted) {
							DeleteCachedAudioFile(full_asset_path, mod_db_folder);
						}
						else if (!HasCachedAudioFile(full_asset_path, mod_db_folder)) {
							if (CacheAudioFile(full_asset_path, mod_db_folder, outdated)) {
								LogInfo("Successfully cached audio file '{}'...", full_asset_path.string());
							}
							else {
								LogInfo("Failed caching audio file '{}'...", full_asset_path.string());
							}
						}
					}
				});
				mod_db.WriteDatabase();
			}

			if (enabled) {
				vfs.MountFolder(mod_folder.string(), prio);
				vfs.MountFolder(mod_db_folder.string(), prio);
			}

			mMods.push_back(std::move(mod_name));
		}

		if (sticker_gen.NeedsRegeneration() || !fs::exists(db_folder / "Data/Textures/journal_stickers.DDS")) {
			if (sticker_gen.GenerateStickers(db_original_folder, db_folder, "Data/Textures/journal_stickers.png", "Data/Textures/journal_entry_people.png", vfs)) {
				LogInfo("Successfully generated sticker and journal entries from installed character mods...");
			}
			else {
				LogInfo("Failed generating sticker and journal entries from installed character mods...");
			}
		}

		if (sprite_sheet_merger.NeedsRegeneration(db_folder)) {
			if (sprite_sheet_merger.GenerateRequiredSheets(db_original_folder, db_folder, vfs)) {
				LogInfo("Successfully generated merged sheets from mods...");
			}
			else {
				LogInfo("Failed generating merged sheets from mods...");
			}
		}

		if (has_outdated_shaders || !fs::exists(db_folder / "shaders.hlsl")) {
			if (MergeShaders(db_original_folder, db_folder, "shaders.hlsl", vfs)) {
				LogInfo("Successfully generated a full shader file from installed shader mods...");
			}
			else {
				LogInfo("Failed generating a full shader file from installed shader mods...");
			}
		}

		if (string_merger.NeedsRegen() || !fs::exists(db_folder / "strings00.str")) {
			if (string_merger.MergeStrings(db_original_folder, db_folder, "strings_hashes.hash", vfs)) {
				LogInfo("Successfully generated a full string file from installed string mods...");
			}
			else {
				LogInfo("Failed generating a full string file from installed string mods...");
			}
		}

		vfs.MountFolder(db_folder.string(), -1);

		{
			struct ModNameAndState {
				std::string Name;
				bool Enabled;
			};
			std::map<std::int64_t, ModNameAndState> mod_prio_to_name;
			for (auto& [mod_name, prio_and_state] : mod_name_to_prio) {
				mod_prio_to_name[prio_and_state.Prio] = { mod_name, prio_and_state.Enabled };
			}

			if (auto load_order_file = std::ofstream{ mods_root_path / "load_order.txt", std::ios::trunc }) {
				for (auto& [prio, mod_name_and_state] : mod_prio_to_name) {
					if (algo::contains(mMods, mod_name_and_state.Name)) {
						if (!mod_name_and_state.Enabled) {
							load_order_file << "--";
						}
						load_order_file << mod_name_and_state.Name << '\n';
					}
				}
			}
		}

		LogInfo("All mods initialized...");
	}
	else {
		const fs::path absolute_mods_root_path{ fs::absolute(mods_root_path) };
		LogFatal("The 'Mods/Packs' folder does not exist, did you do a mistake when installing mods or did you not mean to run Playlunky? "
				 "The folder was expected to be found here: {}", absolute_mods_root_path.string());

		LogInfo("No mods were initialized...");
	}
}
