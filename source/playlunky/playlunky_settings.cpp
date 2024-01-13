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
}
PlaylunkySettings::~PlaylunkySettings() = default;

std::string PlaylunkySettings::GetString(const std::string& category, const std::string& setting, const std::string& default_value) const
{
    std::string value = mSettings->Get(std::move(category), std::move(setting), default_value);
    std::string_view value_view{ value };
    if (value_view.starts_with('"') && value_view.ends_with('"'))
    {
        value_view.remove_prefix(1);
        value_view.remove_suffix(1);
        value = value_view;
    }
    return value;
}
bool PlaylunkySettings::GetBool(const std::string& category, const std::string& setting, bool default_value) const
{
    if (const OverriddenSetting* overridden = algo::find_if(mOverriddenSettings, [&](const OverriddenSetting& overridden)
                                                            { return overridden.Category == category && overridden.Setting == setting; }))
        return overridden->Value;
    return mSettings->GetBoolean(std::move(category), std::move(setting), default_value);
}
int PlaylunkySettings::GetInt(const std::string& category, const std::string& setting, int default_value) const
{
    return mSettings->GetInteger(std::move(category), std::move(setting), default_value);
}
float PlaylunkySettings::GetFloat(const std::string& category, const std::string& setting, float default_value) const
{
    return static_cast<float>(mSettings->GetReal(std::move(category), std::move(setting), default_value));
}

void PlaylunkySettings::SetBool(std::string category, std::string setting, bool value)
{
    OverriddenSetting new_setting{
        .Category{ std::move(category) },
        .Setting{ std::move(setting) },
        .Value{ value },
    };
    algo::erase_if(mOverriddenSettings, [&](const OverriddenSetting& overridden)
                   { return overridden.Category == new_setting.Category && overridden.Setting == new_setting.Setting; });
    mOverriddenSettings.push_back(std::move(new_setting));
}

void PlaylunkySettings::WriteToFile(std::string settings_file) const
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
                                                   KnownSetting{ .Name{ "enable_loose_file_warning" }, .AltCategory{ "settings" }, .DefaultValue{ "true" } },
                                                   KnownSetting{ .Name{ "enable_raw_string_loading" }, .DefaultValue{ "false" } },
                                                   KnownSetting{ .Name{ "disable_asset_caching" }, .DefaultValue{ "false" } },
                                                   KnownSetting{ .Name{ "block_save_game" }, .DefaultValue{ "false" } },
                                                   KnownSetting{ .Name{ "allow_save_game_mods" }, .DefaultValue{ "true" } },
                                                   KnownSetting{ .Name{ "use_playlunky_save" }, .DefaultValue{ "false" } },
                                                   KnownSetting{ .Name{ "disable_steam_achievements" }, .DefaultValue{ "false" } },
                                                   KnownSetting{ .Name{ "speedrun_mode" }, .DefaultValue{ "false" } },
                                                   KnownSetting{ .Name{ "font_file" }, .DefaultValue{ "default" } },
                                                   KnownSetting{ .Name{ "font_file_ru" }, .DefaultValue{ "\"default\"" } },
                                                   KnownSetting{ .Name{ "font_file_jp" }, .DefaultValue{ "\"default\"" } },
                                                   KnownSetting{ .Name{ "font_file_ko" }, .DefaultValue{ "\"default\"" } },
                                                   KnownSetting{ .Name{ "font_file_zhcn" }, .DefaultValue{ "\"default\"" } },
                                                   KnownSetting{ .Name{ "font_file_zhtw" }, .DefaultValue{ "\"default\"" } },
                                                   KnownSetting{ .Name{ "font_file_emoji" }, .DefaultValue{ "\"default\"" } },
                                                   KnownSetting{ .Name{ "font_scale" }, .DefaultValue{ "1.0" } },
                                               } },
        KnownCategory{ { "script_settings" }, {
                                                  KnownSetting{ .Name{ "enable_developer_mode" }, .AltCategory{ "settings" }, .DefaultValue{ "false" } },
                                                  KnownSetting{ .Name{ "enable_developer_console" }, .DefaultValue{ "false" } },
                                                  KnownSetting{ .Name{ "console_history_size" }, .DefaultValue{ "20" } },
                                              } },
        KnownCategory{ { "audio_settings" }, {
                                                 KnownSetting{ .Name{ "enable_loose_audio_files" }, .AltCategory{ "settings" }, .DefaultValue{ "true" } },
                                                 KnownSetting{ .Name{ "cache_decoded_audio_files" }, .AltCategory{ "settings" }, .DefaultValue{ "false" } },
                                                 KnownSetting{ .Name{ "synchronous_update" }, .DefaultValue{ "true" } },
                                             } },
        KnownCategory{ { "sprite_settings" }, {
                                                  KnownSetting{ .Name{ "random_character_select" }, .AltCategory{ "settings" }, .DefaultValue{ "false" } },
                                                  KnownSetting{ .Name{ "link_related_files" }, .DefaultValue{ "true" }, .Comment{ "Makes sure that related files, e.g. char_black.png and char_black.json are always loaded from the same mod" } },
                                                  KnownSetting{ .Name{ "generate_character_journal_stickers" }, .DefaultValue{ "true" } },
                                                  KnownSetting{ .Name{ "generate_character_journal_entries" }, .DefaultValue{ "true" } },
                                                  KnownSetting{ .Name{ "generate_sticker_pixel_art" }, .DefaultValue{ "true" } },
                                                  KnownSetting{ .Name{ "enable_sprite_hot_loading" }, .DefaultValue{ "false" } },
                                                  KnownSetting{ .Name{ "sprite_hot_load_delay" }, .DefaultValue{ "400" }, .Comment{ "Increase this value if you experience crashes when a sprite is reloaded" } },
                                                  KnownSetting{ .Name{ "enable_customizable_sheets" }, .DefaultValue{ "true" }, .Comment{ "Enables the customizable sprite sheets feature, does not work in speedrun mode" } },
                                                  KnownSetting{ .Name{ "enable_luminance_scaling" }, .DefaultValue{ "true" }, .Comment{ "Scales luminance of customized images based on the color" } },
                                              } },
        KnownCategory{ { "bug_fixes" }, {
                                            KnownSetting{ .Name{ "missing_thorns" }, .DefaultValue{ "true" }, .Comment{ "Adds textures for the missing jungle thorns configurations" } },
                                            KnownSetting{ .Name{ "missing_pipes" }, .DefaultValue{ "false" }, .Comment{ "May cause issues in multiplayer. Adds textures for the missing sunken city pipes configurations and makes those pipes work, some requiring user input" } },
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
            /* why did it write overridden settings back to file? sounds like a one-off thing that shouldn't be saved
            if (const OverriddenSetting* overridden = algo::find_if(mOverriddenSettings, [&](const OverriddenSetting& overridden)
                                                                    { return overridden.Category == known_category.Name && overridden.Setting == known_setting.Name; }))
            {
                value = overridden->Value ? "true" : "false";
            }
            */

            // transition to toml-compatible values
            {
                if (value == "on")
                {
                    value = "true";
                }
                else if (value == "off")
                {
                    value = "false";
                }

                std::string_view value_view{ value };
                if (known_setting.Name.starts_with("font_file") && !value_view.starts_with('"') && !value_view.ends_with('"'))
                {
                    value = fmt::format("\"{}\"", value);
                }
            }

            if (const char* comment = algo::find(value, '#'))
            {
                value.erase(value.begin() + (comment - value.data()), value.end());
                value = algo::trim(std::move(value));
            }
            if (!known_setting.Comment.empty())
            {
                ini_output += fmt::format("# {}\n", known_setting.Comment);
            }
            ini_output += fmt::format("{}={}\n", known_setting.Name, value);
        }
        ini_output += '\n';
    }

    if (auto playlunky_ini_output = std::ofstream{ settings_file, std::ios::trunc })
    {
        playlunky_ini_output.write(ini_output.data(), ini_output.size());
    }
}
