#pragma once

#include <filesystem>
#include <string>
#include <vector>

class PlaylunkySettings;
class VirtualFilesystem;

class DmPreviewMerger
{
  public:
    DmPreviewMerger(const PlaylunkySettings& settings);
    DmPreviewMerger(const DmPreviewMerger&) = delete;
    DmPreviewMerger(DmPreviewMerger&&) = delete;
    DmPreviewMerger& operator=(const DmPreviewMerger&) = delete;
    DmPreviewMerger& operator=(DmPreviewMerger&&) = delete;
    ~DmPreviewMerger();

    void RegisterDmLevel(std::filesystem::path path, bool outdated, bool deleted);

    bool NeedsRegeneration(const std::filesystem::path& destination_folder) const;

    bool GenerateDmPreview(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder, VirtualFilesystem& vfs);

  private:
    struct RegisteredDmLevel
    {
        std::filesystem::path Path;
        bool Outdated;
        bool Deleted;
    };
    std::vector<RegisteredDmLevel> mDmLevels;
};
