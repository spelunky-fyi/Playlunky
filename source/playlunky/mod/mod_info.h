#pragma once

#include "sprite_sheet_merger_types.h"

#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class ModDatabase;

class ModInfo
{
  public:
    ModInfo(std::string name);
    ~ModInfo() = default;

    const std::string& GetNameInternal() const
    {
        return mNameInternal;
    }
    const std::string& GetName() const
    {
        return mName;
    }
    const std::string& GetDescription() const
    {
        return mDescription;
    }
    const std::string& GetAuthor() const
    {
        return mAuthor;
    }
    const std::string& GetVersion() const
    {
        return mVersion;
    }

    std::string Dump() const;

    void ReadFromDatabase(const ModDatabase& mod_db);

    void ReadExtendedInfoFromJson(std::string_view path);
    bool HasExtendedInfo() const
    {
        return mExtendedVersionAvailable;
    }

    const CustomImages& GetCustomImages() const
    {
        return mCustomImages;
    }
    bool IsCustomImageSource(std::string_view relative_path) const
    {
        return IsCustomImageSource(std::string{ relative_path });
    }
    bool IsCustomImageSource(const std::string& relative_path) const
    {
        return mCustomImages.contains(relative_path);
    }

    const std::set<std::string>& GetBugFixes() const
    {
        return mBugFixes;
    }
    bool IsBugFixEnabled(const std::string& name) const
    {
        return mBugFixes.contains(name);
    }

  private:
    std::string mNameInternal;
    std::string mName;

    bool mExtendedVersionAvailable{ false };
    std::string mDescription;
    std::string mAuthor;
    std::string mVersion;

    CustomImages mCustomImages;
    std::set<std::string> mBugFixes;
};
