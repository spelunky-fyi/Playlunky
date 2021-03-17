#include <playlunky_settings.h>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4244)
#include <INIReader.h>
#pragma warning(pop)

PlaylunkySettings::PlaylunkySettings(std::string settings_file)
	: mSettings{ new INIReader{ std::move(settings_file) } } {
}
PlaylunkySettings::~PlaylunkySettings() = default;

bool PlaylunkySettings::GetBool(std::string category, std::string setting, bool default_value) const {
	return mSettings->GetBoolean(std::move(category), std::move(setting), default_value);
}
