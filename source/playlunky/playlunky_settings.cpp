#include "playlunky_settings.h"

#include "util/algorithms.h"

#include <array>
#include <fstream>
#include <string_view>
#include <util/format.h>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4244)
#include <INIReader.h>
#pragma warning(pop)

PlaylunkySettings::PlaylunkySettings(std::string settings_file)
    : mSettings{ new INIReader{ std::move(settings_file) } }
{
    struct KnownSetting
    {
        std::string_view Name;
        std::string_view AltCategory;
        std::string_view DefaultValue;
        std::string_view Comment;
    };
    struct KnownCategory
    {
        std::string_view Name;
        std::vector<KnownSetting> Settings;
    };
    std::array known_categories{
        KnownCategory{ { "general_settings" }, {
                                                   KnownSetting{ .Name{ "enable_loose_file_warning" }, .AltCategory{ "settings" }, .DefaultValue{ "on" } },
                                                   KnownSetting{ .Name{ "enable_raw_string_loading" }, .DefaultValue{ "off" } },
                                                   KnownSetting{ .Name{ "disable_asset_caching" }, .DefaultValue{ "off" } },
                                                   KnownSetting{ .Name{ "block_save_game" }, .DefaultValue{ "off" } },
                                                   KnownSetting{ .Name{ "allow_save_game_mods" }, .DefaultValue{ "on" } },
                                                   KnownSetting{ .Name{ "speedrun_mode" }, .DefaultValue{ "off" } },
                                               } },
        KnownCategory{ { "script_settings" }, {
                                                  KnownSetting{ .Name{ "enable_developer_mode" }, .AltCategory{ "settings" }, .DefaultValue{ "off" } },
                                                  KnownSetting{ .Name{ "enable_developer_console" }, .DefaultValue{ "off" } },
                                                  KnownSetting{ .Name{ "console_history_size" }, .DefaultValue{ "20" } },
                                              } },
        KnownCategory{ { "audio_settings" }, {
                                                 KnownSetting{ .Name{ "enable_loose_audio_files" }, .AltCategory{ "settings" }, .DefaultValue{ "on" } },
                                                 KnownSetting{ .Name{ "cache_decoded_audio_files" }, .AltCategory{ "settings" }, .DefaultValue{ "off" } },
                                                 KnownSetting{ .Name{ "synchronous_update" }, .DefaultValue{ "on" } },
                                             } },
        KnownCategory{ { "sprite_settings" }, {
                                                  KnownSetting{ .Name{ "random_character_select" }, .AltCategory{ "settings" }, .DefaultValue{ "off" } },
                                                  KnownSetting{ .Name{ "generate_character_journal_stickers" }, .DefaultValue{ "on" } },
                                                  KnownSetting{ .Name{ "generate_character_journal_entries" }, .DefaultValue{ "on" } },
                                                  KnownSetting{ .Name{ "generate_sticker_pixel_art" }, .DefaultValue{ "on" } },
                                                  KnownSetting{ .Name{ "enable_sprite_hot_loading" }, .DefaultValue{ "off" } },
                                                  KnownSetting{ .Name{ "sprite_hot_load_delay" }, .DefaultValue{ "500" }, .Comment{ "Increase this value if you experience crashes when a sprite is reloaded" } },
                                              } },
        KnownCategory{ { "key_bindings" }, {
                                               KnownSetting{ .Name{ "console" }, .DefaultValue{ "0xc0" }, .Comment{ "Default 0xc0 == ~ for US" } },
                                               KnownSetting{ .Name{ "console_alt" }, .DefaultValue{ "0xdc" }, .Comment{ "Default 0xdc == \\ for US" } },
                                               KnownSetting{ .Name{ "console_close" }, .DefaultValue{ "0x1b" }, .Comment{ "Default 0x1b == ESC" } },
                                           } }
    };

    std::string ini_output;
    for (const KnownCategory& known_category : known_categories)
    {
        ini_output += fmt::format("[{}]\n", known_category.Name);
        for (const KnownSetting& known_setting : known_category.Settings)
        {
            std::string value = mSettings->Get(std::string{ known_category.Name }, std::string{ known_setting.Name }, "");
            if (!known_setting.AltCategory.empty() && value.empty())
            {
                value = mSettings->Get(std::string{ known_setting.AltCategory }, std::string{ known_setting.Name }, "");
            }
            if (value.empty())
            {
                value = std::string{ known_setting.DefaultValue };
            }
            if (const char* comment = algo::find(value, '#'))
            {
                value.erase(value.begin() + (comment - value.data()), value.end());
                value = algo::trim(std::move(value));
            }
            if (known_setting.Comment.empty())
            {
                ini_output += fmt::format("{}={}\n", known_setting.Name, value);
            }
            else
            {
                ini_output += fmt::format("{}={} # {}\n", known_setting.Name, value, known_setting.Comment);
            }
        }
        ini_output += '\n';
    }

    if (auto playlunky_ini_output = std::ofstream{ "playlunky.ini", std::ios::trunc })
    {
        playlunky_ini_output.write(ini_output.data(), ini_output.size());
    }
}
PlaylunkySettings::~PlaylunkySettings() = default;

bool PlaylunkySettings::GetBool(std::string category, std::string setting, bool default_value) const
{
    return mSettings->GetBoolean(std::move(category), std::move(setting), default_value);
}
int PlaylunkySettings::GetInt(std::string category, std::string setting, int default_value) const
{
    return mSettings->GetInteger(std::move(category), std::move(setting), default_value);
}
