#include "unzip_mod.h"

#include "log.h"
#include "util/algorithms.h"
#include "util/regex.h"

static constexpr ctll::fixed_string s_FontRule{ ".*\\.fnb" };
static constexpr std::string_view s_FontTargetPath{ "Data/Fonts" };

static constexpr ctll::fixed_string s_ArenaLevelRule{ "dm.*\\.lvl" };
static constexpr ctll::fixed_string s_ArenaLevelTokRule{ ".*\\.tok" };
static constexpr std::string_view s_ArenaLevelTargetPath{ "Data/Levels/Arena" };

static constexpr ctll::fixed_string s_LevelRule{ ".*\\.lvl" };
static constexpr std::string_view s_LevelTargetPath{ "Data/Levels" };

static constexpr ctll::fixed_string s_OldTextureRule{ "ai\\.(DDS|png)" };
static constexpr std::string_view s_OldTextureTargetPath{ "Data/Textures/OldTextures" };

static constexpr ctll::fixed_string s_TextureRule{ ".*\\.(DDS|png)" };
static constexpr std::string_view s_TextureTargetPath{ "Data/Textures" };

void FixModFolderStructure(const std::filesystem::path& mod_folder) {
	namespace fs = std::filesystem;
	struct PathMapping {
		fs::path CurrentPath;
		fs::path TargetPath;
	};
	std::vector<PathMapping> path_mappings;

	const auto db_folder = mod_folder / ".db";

	for (const auto& path : fs::recursive_directory_iterator(mod_folder)) {
		if (fs::is_regular_file(path) && !algo::is_sub_path(path, db_folder)) {
			const auto file_name = path.path().filename().string();
			if (ctre::match<s_FontRule>(file_name)) {
				path_mappings.push_back({ path, mod_folder / s_FontTargetPath / file_name });
			}
			else if (ctre::match<s_ArenaLevelRule>(file_name) || ctre::match<s_ArenaLevelTokRule>(file_name)) {
				path_mappings.push_back({ path, mod_folder / s_ArenaLevelTargetPath / file_name });
			}
			else if (ctre::match<s_LevelRule>(file_name)) {
				path_mappings.push_back({ path, mod_folder / s_LevelTargetPath / file_name });
			}
			else if (ctre::match<s_OldTextureRule>(file_name)) {
				path_mappings.push_back({ path, mod_folder / s_OldTextureTargetPath / file_name });
			}
			else if (ctre::match<s_TextureRule>(file_name)) {
				path_mappings.push_back({ path, mod_folder / s_TextureTargetPath / file_name });
			}
			else {
				path_mappings.push_back({ path, mod_folder / file_name });
			}
		}
	}

	for (auto& [current_path, target_path] : path_mappings) {
		if (current_path != target_path) {
			{
				const auto target_parent_path = target_path.parent_path();
				if (!fs::exists(target_parent_path)) {
					fs::create_directories(target_parent_path);
				}
			}

			fs::rename(current_path, target_path);
		}
	}
}