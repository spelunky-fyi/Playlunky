#pragma once

#include <memory>
#include <string>

class INIReader;

class PlaylunkySettings
{
  public:
    PlaylunkySettings(std::string settings_file);
    ~PlaylunkySettings();

    std::string GetString(const std::string& category, const std::string& setting, const std::string& default_value) const;
    bool GetBool(const std::string& category, const std::string& setting, bool default_value) const;
    int GetInt(const std::string& category, const std::string& setting, int default_value) const;

    void SetBool(std::string category, std::string setting, bool value);

    void WriteToFile(std::string settings_file) const;

  private:
    std::unique_ptr<INIReader> mSettings;

    struct OverriddenSetting
    {
        std::string Category;
        std::string Setting;
        bool Value;
    };
    std::vector<OverriddenSetting> mOverriddenSettings;
};
