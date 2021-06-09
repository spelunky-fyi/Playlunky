#include "patch_character_definitions.h"

#include "detour/sigfun.h"
#include "detour/sigscan.h"
#include "log.h"
#include "playlunky_settings.h"
#include "util/algorithms.h"
#include "util/format.h"
#include "virtual_filesystem.h"

#pragma warning(push)
#pragma warning(disable : 4927)
#include <FileWatch.hpp>
#pragma warning(pop)

#include <spel2.h>

#include <array>
#include <cassert>
#include <charconv>
#include <codecvt>
#include <fstream>
#include <span>
#include <unordered_map>

std::vector<std::unique_ptr<filewatch::FileWatch<std::filesystem::path>>> g_FileWatchers;

template<typename T>
std::string toUTF8(const std::basic_string<T, std::char_traits<T>>& source)
{
    std::wstring_convert<std::codecvt_utf8_utf16<T>, T> convertor;
    return convertor.to_bytes(source);
}

template<typename T>
std::basic_string<T, std::char_traits<T>, std::allocator<T>> fromUTF8(const std::string& source)
{
    std::wstring_convert<std::codecvt_utf8_utf16<T>, T> convertor;
    return convertor.from_bytes(source);
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
    for (std::uint32_t i = 0; i < characters.size(); i++)
    {
        auto char_name = characters[i];

        auto name_file_name = fmt::format("char_{}.name", char_name);
        if (auto file_path = vfs.GetFilePath(name_file_name))
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
                    std::u16string full_name = fromUTF8<char16_t>(full_name_narrow);
                    Spelunky_SetCharacterFullName(i, full_name.c_str());
                }

                {
                    std::u16string short_name = fromUTF8<char16_t>(short_name_narrow);
                    Spelunky_SetCharacterShortName(i, short_name.c_str());
                }
            }
        }

        auto color_file_name = fmt::format("char_{}.color", char_name);
        if (auto file_path = vfs.GetFilePath(color_file_name))
        {
            auto change_heart_color_from_file = [&, file_path = file_path.value()](const std::filesystem::path&, const filewatch::Event change_type)
            {
                if (change_type == filewatch::Event::removed || change_type == filewatch::Event::renamed_old)
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
            change_heart_color_from_file(file_path.value(), filewatch::Event::added);

            if (settings.GetBool("script_settings", "enable_developer_mode", false))
            {
                g_FileWatchers.push_back(std::make_unique<filewatch::FileWatch<std::filesystem::path>>(file_path.value(), change_heart_color_from_file));
            }
        }
    }
}
