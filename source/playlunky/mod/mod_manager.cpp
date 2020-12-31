#include "mod_manager.h"

#include "character_sticker_gen.h"
#include "extract_game_assets.h"
#include "fix_mod_structure.h"
#include "mod_database.h"
#include "png_dds_conversion.h"
#include "shader_merger.h"
#include "unzip_mod.h"
#include "virtual_filesystem.h"

#include "log.h"
#include "util/algorithms.h"

#include <Windows.h>
#include <zip.h>
#include <ctre.hpp>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <map>

static constexpr ctll::fixed_string s_CharacterRule{ ".+char_(.*)\\.png" };

ModManager::ModManager(std::string_view mods_root, VirtualFilesystem& vfs) {
	namespace fs = std::filesystem;

	LogInfo("Initializing Mod Manger...");

	LogInfo("Scanning for mods...");

	const fs::path mods_root_path{ mods_root };
	if (fs::exists(mods_root_path) && fs::is_directory(mods_root_path)) {
		{
			ModDatabase mod_db{ mods_root, static_cast<ModDatabaseFlags>(ModDatabaseFlags_Files | ModDatabaseFlags_Folders) };

			mod_db.SetEnabled(true);
			mod_db.UpdateDatabase();
			mod_db.ForEachFile([&mods_root_path](const fs::path& rel_file_path, bool outdated, [[maybe_unused]] bool deleted, [[maybe_unused]] std::optional<bool> new_enabled_state) {
				if (outdated) {
					if (rel_file_path.extension() == ".zip") {
						const fs::path zip_path = mods_root_path / rel_file_path;
						const auto message = fmt::format("Found archive '{}' in mods packs folder. Do you want to unzip it in order for it to be loadable as a mod?", zip_path.filename().string());
						if (MessageBox(NULL, message.c_str(), "Zipped Mod Found", MB_YESNO) == IDYES) {
							UnzipMod(zip_path);
						}
					}
				}
			});

			mod_db.UpdateDatabase();
			mod_db.ForEachFolder([&mods_root_path](const fs::path& rel_folder_path, bool outdated, [[maybe_unused]] bool deleted, [[maybe_unused]] std::optional<bool> new_enabled_state) {
				if (outdated) {
					FixModFolderStructure(mods_root_path / rel_folder_path);
				}
			});

			mod_db.WriteDatabase();
		}

		const auto db_folder = mods_root_path / ".db";
		const auto db_original_folder = db_folder / "Original";
		{
			const auto files = std::array{
				fs::path{ "Data/Textures/journal_stickers.DDS" },
				fs::path{ "Data/Textures/journal_entry_people.DDS" },
				fs::path{ "shaders.hlsl" }
			};
			if (ExtractGameAssets(files, db_original_folder)) {
				LogInfo("Successfully extracted required game assets...");
			}
			else {
				LogInfo("Failed extracting required game assets, some features might not function...");
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
							// basically mod_name.trim()
							mod_name.erase(mod_name.begin(), std::find_if(mod_name.begin(), mod_name.end(), [](unsigned char ch) {
								return !std::isspace(ch);
							}));
							enabled = false;
						}
						mod_name_to_prio[std::move(mod_name)] = { static_cast<std::int64_t>(mod_name_to_prio.size()), enabled };
					}
				}
			}

			return mod_name_to_prio;
		}();

		CharacterStickerGenerator sticker_gen;
		bool has_outdated_shaders{ false };

		for (const fs::path& mod_folder : mod_folders) {
			std::string mod_name = mod_folder.stem().string(); // Not const so we can move from it later
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
					if (rel_asset_path.extension() == ".png") {
						const auto full_asset_path = mod_folder / rel_asset_path;
						const auto full_asset_path_string = full_asset_path.string();

						if (auto character_match = ctre::match<s_CharacterRule>(full_asset_path_string)) {
							const std::string_view color = character_match.get<1>().to_view();
							if (!sticker_gen.RegisterCharacter(color, outdated || new_enabled_state.has_value())) {
								const std::string character_file_name = full_asset_path.filename().string();
								LogInfo("Mod '{}' contains an unkown character file '{}'", mod_name, character_file_name);
							}
						}

						if (outdated || deleted) {
							const auto db_destination = (mod_db_folder / rel_asset_path).replace_extension(".dds");

							if (deleted) {
								if (fs::remove(db_destination)) {
									LogInfo("Successfully deleted file '{}' that was removed from a mod...", full_asset_path.string());
								}
							}
							else if (ConvertPngToDds(full_asset_path, db_destination))
							{
								LogInfo("Successfully converted file '{}' to be readable by the game...", full_asset_path.string());
							}
							else {
								LogInfo("Failed converting file '{}' to be readable by the game...", full_asset_path.string());
							}
						}
					}
					else if (rel_asset_path == "shaders_mod.hlsl") {
						has_outdated_shaders = has_outdated_shaders || outdated || new_enabled_state.has_value();
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

		if (has_outdated_shaders || !fs::exists(db_folder / "shaders.hlsl")) {
			if (MergeShaders(db_original_folder, db_folder, "shaders.hlsl", vfs)) {
				LogInfo("Successfully generated a full shader file from installed shader mods...");
			}
			else {
				LogInfo("Failed generating a full shader file from installed shader mods...");
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
	}
}
