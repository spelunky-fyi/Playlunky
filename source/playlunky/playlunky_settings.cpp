#include <playlunky_settings.h>

#include <array>
#include <fstream>
#include <string_view>
#include <vector>
#include <util/format.h>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4244)
#include <INIReader.h>
#pragma warning(pop)

PlaylunkySettings::PlaylunkySettings(std::string settings_file)
	: mSettings{ new INIReader{ std::move(settings_file) } } {
	struct KnownSetting {
		std::string_view Name;
		std::string_view AltCategory;
		std::string_view DefaultValue;
	};
	struct KnownCategory {
		std::string_view Name;
		std::vector<KnownSetting> Settings;
	};
	std::array known_categories{
		KnownCategory{ { "general_settings" }, {
			KnownSetting{ .Name{ "enable_loose_file_warning" }, .AltCategory{ "settings" }, .DefaultValue{ "on" } }
		} },
		KnownCategory{ { "script_settings" }, {
			KnownSetting{ .Name{ "enable_developer_mode" }, .AltCategory{ "settings" }, .DefaultValue{ "off" } }
		} },
		KnownCategory{ { "audio_settings" }, {
			KnownSetting{ .Name{ "enable_loose_audio_files" }, .AltCategory{ "settings" }, .DefaultValue{ "on" } },
			KnownSetting{ .Name{ "cache_decoded_audio_files" }, .AltCategory{ "settings" }, .DefaultValue{ "off" } }
		} },
		KnownCategory{ { "sprite_settings" }, {
			KnownSetting{ .Name{ "random_character_select" }, .AltCategory{ "settings" }, .DefaultValue{ "off" } },
			KnownSetting{ .Name{ "generate_character_journal_stickers" }, .DefaultValue{ "on" } },
			KnownSetting{ .Name{ "generate_character_journal_entries" }, .DefaultValue{ "on" } },
			KnownSetting{ .Name{ "generate_sticker_pixel_art" }, .DefaultValue{ "on" } },
		} }
	};

	std::string ini_output;
	for (const KnownCategory& known_category : known_categories) {
		ini_output += fmt::format("[{}]\n", known_category.Name);
		for (const KnownSetting& known_setting : known_category.Settings) {
			std::string value = mSettings->Get(std::string{ known_category.Name }, std::string{ known_setting.Name }, "");
			if (!known_setting.AltCategory.empty() && value.empty()) {
				value = mSettings->Get(std::string{ known_setting.AltCategory }, std::string{ known_setting.Name }, "");
			}
			if (value.empty()) {
				value = std::string{ known_setting.DefaultValue };
			}
			ini_output += fmt::format("{}={}\n", known_setting.Name, value);
		}
		ini_output += '\n';
	}

	if (auto playlunky_ini_output = std::ofstream{ "playlunky.ini", std::ios::trunc }) {
		playlunky_ini_output.write(ini_output.data(), ini_output.size());
	}
}
PlaylunkySettings::~PlaylunkySettings() = default;

bool PlaylunkySettings::GetBool(std::string category, std::string setting, bool default_value) const {
	return mSettings->GetBoolean(std::move(category), std::move(setting), default_value);
}
