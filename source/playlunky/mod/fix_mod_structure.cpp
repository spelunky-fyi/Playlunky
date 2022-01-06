#include "unzip_mod.h"

#include "known_files.h"
#include "log.h"
#include "util/algorithms.h"
#include "util/regex.h"

#include <optional>

static constexpr ctll::fixed_string s_FontRule{ ".*\\.fnb" };
static constexpr std::string_view s_FontTargetPath{ "Data/Fonts" };

static constexpr ctll::fixed_string s_ArenaLevelRule{ "dm.*\\.lvl" };
static constexpr ctll::fixed_string s_ArenaLevelTokRule{ ".*\\.tok" };
static constexpr std::string_view s_ArenaLevelTargetPath{ "Data/Levels/Arena" };

static constexpr ctll::fixed_string s_LevelRule{ ".*\\.lvl" };
static constexpr std::string_view s_LevelTargetPath{ "Data/Levels" };

static constexpr ctll::fixed_string s_ColorTextureRule{ ".*_col\\.(dds|bmp|dib|jpeg|jpg|jpe|jp2|png|webp|pbm|pgm|ppm|sr|ras|tiff|tif)" };
static constexpr ctll::fixed_string s_LuminosityTextureRule{ ".*_lumin\\.(dds|bmp|dib|jpeg|jpg|jpe|jp2|png|webp|pbm|pgm|ppm|sr|ras|tiff|tif)" };

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

std::optional<std::filesystem::path> GetCorrectPath(const std::filesystem::path& file_path)
{
    const auto file_name_path = [&file_path]()
    {
        if (algo::is_same_path(file_path.extension(), ".dds"))
        {
            return file_path.filename().replace_extension(".DDS");
        }
        return file_path.filename();
    }();
    const auto file_name = file_name_path.string();

    const auto file_name_lower = algo::to_lower(file_name);
    const auto file_stem = file_path.stem().string();

    if (ctre::match<s_FontRule>(file_name))
    {
        return s_FontTargetPath / file_name_path;
    }
    else if (ctre::match<s_ArenaLevelRule>(file_name) || ctre::match<s_ArenaLevelTokRule>(file_name))
    {
        if (algo::contains(s_ArenaLevelFiles, file_stem))
        {
            return s_ArenaLevelTargetPath / file_name_path;
        }
    }
    else if (ctre::match<s_LevelRule>(file_name))
    {
        if (algo::contains(s_LevelFiles, file_stem))
        {
            return s_LevelTargetPath / file_name_path;
        }
    }
    else if (ctre::match<s_TextureRule>(file_name_lower))
    {
        if (ctre::match<s_ColorTextureRule>(file_name_lower) || ctre::match<s_LuminosityTextureRule>(file_name_lower))
        {
            const auto size = ctre::match<s_ColorTextureRule>(file_name_lower) ? 4 : 5;

            const auto old_file_name = file_path.filename();
            const auto old_stem = file_path.stem().string();
            const auto new_file_name = old_stem.substr(0, old_stem.size() - size) + file_path.extension().string();
            if (auto correct_path = GetCorrectPath(std::filesystem::path{ file_path }.replace_filename(new_file_name)))
            {
                correct_path.value().replace_filename(old_file_name);
                return correct_path;
            }
            return std::nullopt;
        }
        else if (ctre::match<s_OldTextureRule>(file_name_lower))
        {
            return s_OldTextureTargetPath / file_name_path;
        }
        else if (ctre::match<s_FullTextureRule>(file_name_lower))
        {
            return s_FullTextureTargetPath / file_name_path;
        }
        else if (algo::contains(s_PetsEntityFiles, file_stem))
        {
            return s_FullTextureTargetPath / ("Pets" / file_name_path);
        }
        else if (algo::contains(s_MountsEntityFiles, file_stem))
        {
            return s_FullTextureTargetPath / ("Mounts" / file_name_path);
        }
        else if (algo::contains(s_GhostEntityFiles, file_stem))
        {
            return s_FullTextureTargetPath / ("Ghost" / file_name_path);
        }
        else if (algo::contains(s_CrittersEntityFiles, file_stem))
        {
            return s_FullTextureTargetPath / ("Critters" / file_name_path);
        }
        else if (algo::contains(s_MonstersEntityFiles, file_stem))
        {
            return s_FullTextureTargetPath / ("Monsters" / file_name_path);
        }
        else if (algo::contains(s_BigMonstersEntityFiles, file_stem))
        {
            return s_FullTextureTargetPath / ("BigMonsters" / file_name_path);
        }
        else if (algo::contains(s_PeopleEntityFiles, file_stem))
        {
            return s_FullTextureTargetPath / ("People" / file_name_path);
        }
        else if (algo::contains(s_DecorationsEntityFiles, file_stem))
        {
            return s_FullTextureTargetPath / ("Decorations" / file_name_path);
        }
        else if (algo::contains(s_KnownTextureFiles, file_stem))
        {
            return s_TextureTargetPath / file_name_path;
        }
    }
    else if (algo::contains(s_KnownAudioFiles, file_stem))
    {
        if (ctre::match<s_WavRule>(file_name))
        {
            return s_WavTargetPath / file_name_path;
        }
        else if (ctre::match<s_OggRule>(file_name))
        {
            return s_OggTargetPath / file_name_path;
        }
        else if (ctre::match<s_Mp3Rule>(file_name))
        {
            return s_Mp3TargetPath / file_name_path;
        }
        else if (ctre::match<s_WvRule>(file_name))
        {
            return s_WvTargetPath / file_name_path;
        }
        else if (ctre::match<s_OpusRule>(file_name))
        {
            return s_OpusTargetPath / file_name_path;
        }
        else if (ctre::match<s_FlacRule>(file_name))
        {
            return s_FlacTargetPath / file_name_path;
        }
        else if (ctre::match<s_MpcRule>(file_name))
        {
            return s_MpcTargetPath / file_name_path;
        }
        else if (ctre::match<s_MppRule>(file_name))
        {
            return s_MppTargetPath / file_name_path;
        }
        else if (algo::contains(s_RestKnownFiles, file_name))
        {
            return file_name;
        }
    }
    else if (algo::contains(s_RestKnownFiles, file_name))
    {
        return file_name;
    }

    return std::nullopt;
}

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
            if (const auto correct_path = GetCorrectPath(path))
            {
                path_mappings.push_back({ path, mod_folder / std::move(correct_path).value() });
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
