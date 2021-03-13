#include "util/unzip_file.h"
#include "util/format.h"

#include <iostream>
#include <filesystem>


namespace fs = std::filesystem;

int main(int argc, char** argv) {
	if (argc == 1) {
		return 2;
	}

	fmt::print("Installing mod from files...\n");

	{
		if (fs::current_path().stem() != "Spelunky 2") {
			const fs::path exe_path{ fs::path{ argv[0] }.parent_path() };
			if (exe_path.stem() != "Spelunky 2") {
				fmt::print("Not running from game folder, please move this executable to the game folder...\n");
				std::system("pause");
				return 1;
			}
			fs::current_path(exe_path);
		}
	}

	std::vector<fs::path> paths{ argv + 1, argv + argc };

	const fs::path mod_dir{ [&paths]() {
		fs::path mod_dir;
		if (paths.size() == 1) {
			const fs::path& only_path = paths.front();
			if (fs::is_directory(only_path)) {
				mod_dir = "Mods/Packs" / only_path.stem();

			}
			else if (only_path.extension() == ".zip") {
				mod_dir = "Mods/Packs" / paths.front().stem();
			}
		}
		if (mod_dir.empty()) {
			const std::size_t first_new_mod = []() {
				std::size_t first_new_mod{ 0 };
				while (fs::exists(fmt::format("Mods/Packs/Mod_{}", first_new_mod))) {
					++first_new_mod;
				}
				return first_new_mod;
			}();
			mod_dir = fmt::format("Mods/Packs/Mod_{}", first_new_mod);
		}
		fs::create_directories(mod_dir);
		return mod_dir;
	}() };

	fmt::print("Installing all files into '{}'...\n", mod_dir.string());

	for (fs::path path : paths) {
		if (path.extension() == ".zip") {
			fmt::print("Unzipping '{}' into '{}'...\n", path.string(), mod_dir.string());
			UnzipFile(path, mod_dir);
		}
		else {
			const fs::path target_path = path.filename().empty()
				? mod_dir / path.parent_path().filename()
				: mod_dir / path.filename();
			fmt::print("Installing '{}' to '{}'...\n", path.string(), target_path.string());
			fs::copy(path, target_path, fs::copy_options::recursive);
		}
	}

	fmt::print("Installation was successfull...\n");
	std::system("pause");
	return 0;
}
