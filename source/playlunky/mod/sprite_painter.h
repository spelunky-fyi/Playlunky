#pragma once

#include <filesystem>
#include <memory>
#include <mutex>
#include <vector>

class PlaylunkySettings;
class SpriteSheetMerger;
class VirtualFilesystem;

class SpritePainter
{
  public:
    SpritePainter(SpriteSheetMerger& merger, VirtualFilesystem& vfs, const PlaylunkySettings& settings);
    SpritePainter(const SpritePainter&) = delete;
    SpritePainter(SpritePainter&&) = delete;
    SpritePainter& operator=(const SpritePainter&) = delete;
    SpritePainter& operator=(SpritePainter&&) = delete;
    ~SpritePainter();

    void RegisterSheet(std::filesystem::path full_path, std::filesystem::path db_destination, bool outdated, bool deleted);

    void FinalizeSetup(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder);

    void Update(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder);

  private:
    bool RepaintImage(const std::filesystem::path& full_path, const std::filesystem::path& db_destination);

    struct FilePair
    {
        std::filesystem::path relative_path;
        std::filesystem::path db_destination;
    };
    FilePair ConvertToRealFilePair(const std::filesystem::path& full_path, const std::filesystem::path& db_destination);
    std::optional<std::filesystem::path> GetSourcePath(const std::filesystem::path& relative_path);
    std::filesystem::path ReplaceColExtension(std::filesystem::path path, std::string_view replacement = "");

    struct RegisteredColorModSheet
    {
        std::filesystem::path full_path;
        std::filesystem::path db_destination;
        bool outdated;
    };
    struct PendingRepaint
    {
        const RegisteredColorModSheet* sheet;
        bool has_warned{ false };
    };

    SpriteSheetMerger& m_Merger;
    VirtualFilesystem& m_Vfs;

    std::vector<RegisteredColorModSheet> m_RegisteredColorModSheets;

    std::vector<PendingRepaint> m_PendingRepaints;
    std::size_t m_ReloadTimestamp{ 0 };
    bool m_HasPendingReloads{ false };
};
