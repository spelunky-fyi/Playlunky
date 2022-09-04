#include "patch_character_definitions.h"

#include "detour/sigfun.h"
#include "detour/sigscan.h"
#include "log.h"
#include "playlunky_settings.h"
#include "util/algorithms.h"
#include "util/file_watch.h"
#include "util/format.h"
#include "virtual_filesystem.h"

#include <nlohmann/json.hpp>
#include <spel2.h>

#include <array>
#include <cassert>
#include <charconv>
#include <fstream>
#include <span>
#include <unordered_map>

#include "util/algorithms.h"

struct CharacterDefinition
{
    std::optional<std::string> FullName;
    std::optional<std::string> ShortName;
    std::optional<std::array<float, 4>> Color;
    enum GenderVar
    {
        Male,
        Female
    };
    std::optional<GenderVar> Gender;
};
void from_json(const nlohmann::json& j, CharacterDefinition& char_def)
{
    char_def = CharacterDefinition{};

    if (j.contains("full_name"))
    {
        char_def.FullName = j["full_name"];
    }

    if (j.contains("short_name"))
    {
        char_def.ShortName = j["short_name"];
    }

    if (j.contains("color"))
    {
        auto j_color = j["color"];
        auto color = std::array<float, 4>{};
        if (j_color.is_array() && j_color.size() >= 3)
        {
            color[0] = j_color[0];
            color[1] = j_color[1];
            color[2] = j_color[2];
            if (j_color.size() > 3)
            {
                color[3] = j_color[3];
            }
            char_def.Color = color;
        }
        else if (j_color.is_object())
        {
            color[0] = j_color["red"];
            color[1] = j_color["green"];
            color[2] = j_color["blue"];
            if (j_color.contains("alpha"))
            {
                color[3] = j_color["alpha"];
            }
            char_def.Color = color;
        }
        else if (j_color.is_number_integer() || j_color.is_string())
        {
            std::uint32_t u_color;
            if (j_color.is_number_integer())
            {
                u_color = j_color;
            }
            else
            {
                std::string s_color = j_color;
                size_t start = 0;
                if (s_color.starts_with("0x") || s_color.starts_with("0X"))
                {
                    start = 2;
                }
                else if (s_color.starts_with("#"))
                {
                    start = 1;
                }
                std::from_chars(s_color.data() + start, s_color.data() + s_color.size(), u_color, 16);
            }
            color[0] = static_cast<float>((u_color >> 16) & 0xff) / 255.0f;
            color[1] = static_cast<float>((u_color >> 8) & 0xff) / 255.0f;
            color[2] = static_cast<float>((u_color >> 0) & 0xff) / 255.0f;
            color[3] = 1.0f;
            char_def.Color = color;
        }
    }

    if (j.contains("gender"))
    {
        char_def.Gender = j["gender"] == "Male" || j["gender"] == "male" || j["gender"] == "man"
                              ? CharacterDefinition::Male
                              : CharacterDefinition::Female;
    }
}

void PatchCharacterDefinitions(VirtualFilesystem& vfs, const PlaylunkySettings& settings)
{
    using namespace std::string_view_literals;
    constexpr std::array characters{
        "yellow"sv,
        "magenta"sv,
        "cyan"sv,
        "black"sv,
        "cinnabar"sv,
        "green"sv,
        "olive"sv,
        "white"sv,
        "cerulean"sv,
        "blue"sv,
        "lime"sv,
        "lemon"sv,
        "iris"sv,
        "gold"sv,
        "red"sv,
        "pink"sv,
        "violet"sv,
        "gray"sv,
        "khaki"sv,
        "orange"sv
    };
    const bool random_character_select{ settings.GetBool("sprite_settings", "random_character_select", false) };
    auto vfs_get_file_path = [=, &vfs](const std::string& file_path)
    {
        if (!random_character_select)
        {
            return vfs.GetFilePath(file_path);
        }
        else
        {
            return vfs.GetRandomFilePath(file_path);
        }
    };
    for (std::uint32_t i = 0; i < characters.size(); i++)
    {
        auto char_name = characters[i];

        auto name_file_name = fmt::format("char_{}.name", char_name);
        if (auto file_path = vfs_get_file_path(name_file_name))
        {
            if (auto strings_source_file = std::ifstream{ file_path.value() })
            {
                std::string full_name_narrow;
                std::string short_name_narrow;

                std::getline(strings_source_file, full_name_narrow);
                if (!strings_source_file.eof())
                {
                    std::getline(strings_source_file, short_name_narrow);
                }

                {
                    full_name_narrow = algo::trim(full_name_narrow);
                    const char full_name_str[]{ "FullName:" };
                    const auto colon_pos = full_name_narrow.find(full_name_str);
                    if (colon_pos == 0)
                    {
                        full_name_narrow = algo::trim(full_name_narrow.substr(sizeof(full_name_str) - 1));
                    }
                }

                if (short_name_narrow.empty())
                {
                    short_name_narrow = full_name_narrow;
                    const auto space = short_name_narrow.find(' ');
                    if (space != std::string::npos)
                    {
                        short_name_narrow = algo::trim(short_name_narrow.substr(0, space + 1));
                    }
                }
                else
                {
                    short_name_narrow = algo::trim(short_name_narrow);
                    const char short_name_str[]{ "ShortName:" };
                    const auto colon_pos = short_name_narrow.find(short_name_str);
                    if (colon_pos != std::string::npos)
                    {
                        short_name_narrow = algo::trim(short_name_narrow.substr(sizeof(short_name_str) - 1));
                    }
                }

                {
                    std::u16string full_name = algo::from_utf8<char16_t>(full_name_narrow);
                    Spelunky_SetCharacterFullName(i, full_name.c_str());
                }

                {
                    std::u16string short_name = algo::from_utf8<char16_t>(short_name_narrow);
                    Spelunky_SetCharacterShortName(i, short_name.c_str());
                }
            }
        }

        auto color_file_name = fmt::format("char_{}.color", char_name);
        if (auto file_path = vfs_get_file_path(color_file_name))
        {
            auto change_heart_color_from_file = [i, file_path = file_path.value()](const std::filesystem::path&, const FileEvent change_type)
            {
                if (change_type == FileEvent::Removed || change_type == FileEvent::RenamedOld)
                {
                    return;
                }

                if (auto character_source_file = std::ifstream{ file_path })
                {
                    float color[4]{};
                    try
                    {
                        character_source_file >> color[0] >> color[1] >> color[2];
                    }
                    catch (...)
                    {
                        LogError("Character color file '{}' has bad formatting, expected `float float float`", file_path.string());
                    }

                    if (!character_source_file.eof())
                    {
                        character_source_file >> color[3];
                    }
                    else
                    {
                        color[3] = 1.0f;
                    }

                    Spelunky_SetCharacterHeartColor(i, color);
                }
            };
            change_heart_color_from_file(file_path.value(), FileEvent::Added);

            if (settings.GetBool("script_settings", "enable_developer_mode", false))
            {
                AddFileWatch(file_path.value(), change_heart_color_from_file);
            }
        }

        auto json_file_name = fmt::format("char_{}.json", char_name);
        if (auto file_path = vfs_get_file_path(json_file_name))
        {
            auto apply_char_def_from_json = [i, file_path = file_path.value()](const std::filesystem::path&, const FileEvent change_type)
            {
                if (change_type == FileEvent::Removed || change_type == FileEvent::RenamedOld)
                {
                    return;
                }

                if (auto char_def_file = std::ifstream{ file_path })
                {
                    auto char_def_json = nlohmann::json::parse(char_def_file);
                    CharacterDefinition char_def = char_def_json;
                    if (char_def.FullName)
                    {
                        Spelunky_SetCharacterFullName(i, algo::from_utf8<char16_t>(char_def.FullName.value()).c_str());
                    }
                    if (char_def.ShortName)
                    {
                        Spelunky_SetCharacterShortName(i, algo::from_utf8<char16_t>(char_def.ShortName.value()).c_str());
                    }
                    if (char_def.Color)
                    {
                        float color[4];
                        std::copy(char_def.Color->begin(), char_def.Color->end(), color);
                        Spelunky_SetCharacterHeartColor(i, color);
                    }
                    if (char_def.Gender)
                    {
                        Spelunky_SetCharacterGender(i, char_def.Gender == CharacterDefinition::Female);
                    }
                }
            };
            apply_char_def_from_json(file_path.value(), FileEvent::Added);

            if (settings.GetBool("script_settings", "enable_developer_mode", false))
            {
                AddFileWatch(file_path.value(), apply_char_def_from_json);
            }
        }
    }
}
