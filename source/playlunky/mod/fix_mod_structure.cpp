#include "unzip_mod.h"

#include "known_files.h"
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

static constexpr ctll::fixed_string s_OldTextureRule{ "ai\\.(dds|bmp|dib|jpeg|jpg|jpe|jp2|png|webp|pbm|pgm|ppm|sr|ras|tiff|tif)" };
static constexpr std::string_view s_OldTextureTargetPath{ "Data/Textures/OldTextures" };

static constexpr ctll::fixed_string s_FullTextureRule{ ".*_full\\.(dds|bmp|dib|jpeg|jpg|jpe|jp2|png|webp|pbm|pgm|ppm|sr|ras|tiff|tif)" };
static constexpr std::string_view s_FullTextureTargetPath{ "Data/Textures/Entities" };

static constexpr ctll::fixed_string s_TextureRule{ ".*\\.(dds|bmp|dib|jpeg|jpg|jpe|jp2|png|webp|pbm|pgm|ppm|sr|ras|tiff|tif)" };
static constexpr std::string_view s_TextureTargetPath{ "Data/Textures" };

static constexpr ctll::fixed_string s_WavRule{ ".*\\.wav" };
static constexpr std::string_view s_WavTargetPath{ "soundbank/wav" };

static constexpr ctll::fixed_string s_OggRule{ ".*\\.ogg" };
static constexpr std::string_view s_OggTargetPath{ "soundbank/ogg" };

static constexpr ctll::fixed_string s_Mp3Rule{ ".*\\.mp3" };
static constexpr std::string_view s_Mp3TargetPath{ "soundbank/mp3" };

static constexpr ctll::fixed_string s_WvRule{ ".*\\.wv" };
static constexpr std::string_view s_WvTargetPath{ "soundbank/wv" };

static constexpr ctll::fixed_string s_OpusRule{ ".*\\.opus" };
static constexpr std::string_view s_OpusTargetPath{ "soundbank/opus" };

static constexpr ctll::fixed_string s_FlacRule{ ".*\\.flac" };
static constexpr std::string_view s_FlacTargetPath{ "soundbank/flac" };

static constexpr ctll::fixed_string s_MpcRule{ ".*\\.mpc" };
static constexpr std::string_view s_MpcTargetPath{ "soundbank/mpc" };

static constexpr ctll::fixed_string s_MppRule{ ".*\\.mpp" };
static constexpr std::string_view s_MppTargetPath{ "soundbank/mpp" };

void FixModFolderStructure(const std::filesystem::path& mod_folder)
{
    namespace fs = std::filesystem;
    struct PathMapping
    {
        fs::path CurrentPath;
        fs::path TargetPath;
    };
    std::vector<PathMapping> path_mappings;

    const auto db_folder = mod_folder / ".db";

    for (const auto& path : fs::recursive_directory_iterator(mod_folder))
    {
        if (fs::is_regular_file(path) && !algo::is_sub_path(path, db_folder))
        {
            const auto file_name = [&path]()
            {
                if (algo::is_same_path(path.path().extension(), ".dds"))
                {
                    return path.path().filename().replace_extension(".DDS").string();
                }
                return path.path().filename().string();
            }();
            const auto file_name_lower = algo::to_lower(file_name);
            const auto file_stem = path.path().stem().string();
            if (ctre::match<s_FontRule>(file_name))
            {
                path_mappings.push_back({ path, mod_folder / s_FontTargetPath / file_name });
            }
            else if (ctre::match<s_ArenaLevelRule>(file_name) || ctre::match<s_ArenaLevelTokRule>(file_name))
            {
                path_mappings.push_back({ path, mod_folder / s_ArenaLevelTargetPath / file_name });
            }
            else if (ctre::match<s_LevelRule>(file_name))
            {
                path_mappings.push_back({ path, mod_folder / s_LevelTargetPath / file_name });
            }
            else if (ctre::match<s_TextureRule>(file_name_lower))
            {
                if (ctre::match<s_OldTextureRule>(file_name_lower))
                {
                    path_mappings.push_back({ path, mod_folder / s_OldTextureTargetPath / file_name });
                }
                else if (ctre::match<s_FullTextureRule>(file_name_lower))
                {
                    path_mappings.push_back({ path, mod_folder / s_FullTextureTargetPath / file_name });
                }
                else if (algo::contains(s_PetsEntityFiles, file_stem))
                {
                    path_mappings.push_back({ path, mod_folder / s_FullTextureTargetPath / "Pets" / file_name });
                }
                else if (algo::contains(s_MountsEntityFiles, file_stem))
                {
                    path_mappings.push_back({ path, mod_folder / s_FullTextureTargetPath / "Mounts" / file_name });
                }
                else if (algo::contains(s_GhostEntityFiles, file_stem))
                {
                    path_mappings.push_back({ path, mod_folder / s_FullTextureTargetPath / "Ghost" / file_name });
                }
                else if (algo::contains(s_CrittersEntityFiles, file_stem))
                {
                    path_mappings.push_back({ path, mod_folder / s_FullTextureTargetPath / "Critters" / file_name });
                }
                else if (algo::contains(s_MonstersEntityFiles, file_stem))
                {
                    path_mappings.push_back({ path, mod_folder / s_FullTextureTargetPath / "Monsters" / file_name });
                }
                else if (algo::contains(s_BigMonstersEntityFiles, file_stem))
                {
                    path_mappings.push_back({ path, mod_folder / s_FullTextureTargetPath / "BigMonsters" / file_name });
                }
                else if (algo::contains(s_PeopleEntityFiles, file_stem))
                {
                    path_mappings.push_back({ path, mod_folder / s_FullTextureTargetPath / "People" / file_name });
                }
                else if (algo::contains(s_KnownTextureFiles, file_stem))
                {
                    path_mappings.push_back({ path, mod_folder / s_TextureTargetPath / file_name });
                }
            }
            else if (algo::contains(s_KnownAudioFiles, file_stem))
            {
                if (ctre::match<s_WavRule>(file_name))
                {
                    path_mappings.push_back({ path, mod_folder / s_WavTargetPath / file_name });
                }
                else if (ctre::match<s_OggRule>(file_name))
                {
                    path_mappings.push_back({ path, mod_folder / s_OggTargetPath / file_name });
                }
                else if (ctre::match<s_Mp3Rule>(file_name))
                {
                    path_mappings.push_back({ path, mod_folder / s_Mp3TargetPath / file_name });
                }
                else if (ctre::match<s_WvRule>(file_name))
                {
                    path_mappings.push_back({ path, mod_folder / s_WvTargetPath / file_name });
                }
                else if (ctre::match<s_OpusRule>(file_name))
                {
                    path_mappings.push_back({ path, mod_folder / s_OpusTargetPath / file_name });
                }
                else if (ctre::match<s_FlacRule>(file_name))
                {
                    path_mappings.push_back({ path, mod_folder / s_FlacTargetPath / file_name });
                }
                else if (ctre::match<s_MpcRule>(file_name))
                {
                    path_mappings.push_back({ path, mod_folder / s_MpcTargetPath / file_name });
                }
                else if (ctre::match<s_MppRule>(file_name))
                {
                    path_mappings.push_back({ path, mod_folder / s_MppTargetPath / file_name });
                }
                else if (algo::contains(s_RestKnownFiles, file_name))
                {
                    path_mappings.push_back({ path, mod_folder / file_name });
                }
            }
            else if (algo::contains(s_RestKnownFiles, file_name))
            {
                path_mappings.push_back({ path, mod_folder / file_name });
            }
        }
    }

    for (auto& [current_path, target_path] : path_mappings)
    {
        if (current_path != target_path)
        {
            {
                const auto target_parent_path = target_path.parent_path();
                if (!fs::exists(target_parent_path))
                {
                    fs::create_directories(target_parent_path);
                }
            }

            fs::rename(current_path, target_path);
        }
    }
}
