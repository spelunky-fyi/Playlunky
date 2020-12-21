#include "mod_manager.h"

#include "log.h"
#include "algorithms.h"

#include <filesystem>

ModManager::ModManager(std::string_view mods_root) {
	namespace fs = std::filesystem;

	LogInfo("Initializing Mod Manger...");

	LogInfo("Scanning for mods...");

	const fs::path mods_root_path{ std::move(mods_root) };
	if (fs::exists(mods_root_path) && fs::is_directory(mods_root_path)) {
		const std::vector<fs::path> mod_folders = [this](const fs::path& root_folder) {
			std::vector<fs::path> mod_folders;

			// Note: fully-qualified type on root_folder only because of a bug in MSVC-16.7.1
			auto collect_mod_folders = [&mod_folders](const std::filesystem::path& root_folder, auto& self) -> void {
				for (fs::path sub_path : fs::directory_iterator{ root_folder }) {
					const std::string folder_name = sub_path.stem().string();
					if (algo::contains(mReservedFolders, folder_name)) {
						continue;
					}
					else if (fs::is_directory(sub_path)) {
						if (algo::contains(mRecurseFolders, folder_name)) {
							self(sub_path, self);
						}
						else {
							mod_folders.push_back(std::move(sub_path));
						}
					}
				}
			};

			collect_mod_folders(root_folder, collect_mod_folders);

			return mod_folders;
		}(mods_root_path);


	}
}
