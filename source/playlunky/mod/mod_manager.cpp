#include "mod_manager.h"

#include "mod_database.h"
#include "png_dds_conversion.h"
#include "unzip_mod.h"
#include "virtual_filesystem.h"

#include "log.h"
#include "util/algorithms.h"

#include <Windows.h>
#include <zip.h>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <map>

ModManager::ModManager(std::string_view mods_root, VirtualFilesystem& vfs) {
	namespace fs = std::filesystem;

	LogInfo("Initializing Mod Manger...");

	LogInfo("Scanning for mods...");

	const fs::path mods_root_path{ mods_root };
	if (fs::exists(mods_root_path) && fs::is_directory(mods_root_path)) {
		{
			ModDatabase mod_db{ mods_root, ModDatabaseFlags_Files };
			mod_db.UpdateDatabase();
			mod_db.ForEachOutdatedFile([&mods_root_path](const fs::path& rel_file_path) {
				if (rel_file_path.extension() == ".zip") {
					const fs::path zip_path = mods_root_path / rel_file_path;
					const auto message = fmt::format("Found archive '{}' in mods packs folder. Do you want to unzip it in order for it to be loadable as a mod?", zip_path.filename().string());
					if (MessageBox(NULL, message.c_str(), "Zipped Mod Found", MB_YESNO) == IDYES) {
						UnzipMod(zip_path);
					}
				}
			});
			mod_db.WriteDatabase();
		}

		const std::vector<fs::path> mod_folders = [this](const fs::path& root_folder) {
			std::vector<fs::path> mod_folders;

			for (fs::path sub_path : fs::directory_iterator{ root_folder }) {
				const std::string folder_name = sub_path.stem().string();
				if (fs::is_directory(sub_path)) {
					mod_folders.push_back(sub_path);
				}
			}

			return mod_folders;
		}(mods_root_path);

		std::unordered_map<std::string, std::int64_t> mod_name_to_prio = [&mods_root_path]() {
			std::unordered_map<std::string, std::int64_t> mod_name_to_prio;

			if (auto load_order_file = std::ifstream{ mods_root_path / "load_order.txt" }) {
				while (!load_order_file.eof()) {
					std::string mod_name;
					load_order_file >> mod_name;
					mod_name_to_prio[std::move(mod_name)] = mod_name_to_prio.size();
				}
			}

			return mod_name_to_prio;
		}();

		for (const fs::path& mod_folder : mod_folders) {
			const auto db_folder = mod_folder / ".db";

			{
				ModDatabase mod_db{ mod_folder, static_cast<ModDatabaseFlags>(ModDatabaseFlags_Files | ModDatabaseFlags_Recurse) };
				mod_db.UpdateDatabase();
				mod_db.ForEachOutdatedFile([&mod_folder, db_folder](const fs::path& rel_asset_path) {
					if (rel_asset_path.extension() == ".png") {
						const auto full_asset_path = mod_folder / rel_asset_path;
						const auto db_destination = (db_folder / rel_asset_path).replace_extension(".dds");

						if (ConvertPngToDds(full_asset_path, db_destination))
						{
							LogInfo("Successfully converted file '{}' to be readable by the game...", full_asset_path.string());
						}
						else {
							LogInfo("Failed converting file '{}' to be readable by the game...", full_asset_path.string());
						}
					}
				});
				mod_db.WriteDatabase();
			}

			std::string mod_name = mod_folder.stem().string();
			std::int64_t prio;
			if (mod_name_to_prio.contains(mod_name)) {
				prio = mod_name_to_prio[mod_name];
			}
			else {
				prio = mod_name_to_prio.size();
				mod_name_to_prio[mod_name] = prio;
			}
			vfs.MountFolder(mod_folder.string(), prio);
			vfs.MountFolder(db_folder.string(), prio);

			mMods.push_back(std::move(mod_name));
		}

		{
			std::map<std::int64_t, std::string> mod_prio_to_name;
			for (auto& [mod_name, prio] : mod_name_to_prio) {
				mod_prio_to_name[prio] = mod_name;
			}

			if (auto load_order_file = std::ofstream{ mods_root_path / "load_order.txt", std::ios::trunc }) {
				for (auto& [prio, mod_name] : mod_prio_to_name) {
					if (algo::contains(mMods, mod_name)) {
						load_order_file << mod_name << '\n';
					}
				}
			}
		}
	}
}
