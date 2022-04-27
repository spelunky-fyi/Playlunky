#pragma once

#include <memory>
#include <string>

class INIReader;

class PlaylunkySettings
{
  public:
    PlaylunkySettings(std::string settings_file);
    ~PlaylunkySettings();

    bool GetBool(std::string category, std::string setting, bool default_value) const;
    int GetInt(std::string category, std::string setting, int default_value) const;

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
