#pragma once

#include <filesystem>
#include <memory>
#include <mutex>
#include <vector>

class PlaylunkySettings;
class SpriteSheetMerger;
class VirtualFilesystem;

namespace filewatch
{
template<class T>
class FileWatch;
}

class SpriteHotLoader
{
  public:
    SpriteHotLoader(SpriteSheetMerger& merger, const PlaylunkySettings& settings);
    SpriteHotLoader(const SpriteHotLoader&) = delete;
    SpriteHotLoader(SpriteHotLoader&&) = delete;
    SpriteHotLoader& operator=(const SpriteHotLoader&) = delete;
    SpriteHotLoader& operator=(SpriteHotLoader&&) = delete;
    ~SpriteHotLoader();

    void RegisterSheet(std::filesystem::path full_path, std::filesystem::path db_destination);

    void FinalizeSetup();

    void Update(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder, VirtualFilesystem& vfs);

  private:
    bool PrepareHotLoad(const std::filesystem::path& full_path, const std::filesystem::path& db_destination, bool emit_info);

    struct RegisteredSheet
    {
        std::filesystem::path full_path;
        std::filesystem::path db_destination;
    };
    struct PendingReload
    {
        const RegisteredSheet* sheet;
        bool has_warned{ false };
    };

    SpriteSheetMerger& m_Merger;
    const std::uint32_t m_ReloadDelay;

    std::vector<RegisteredSheet> m_RegisteredSheets;
    std::vector<std::unique_ptr<filewatch::FileWatch<std::filesystem::path>>> m_FileWatchers;

    std::mutex m_PendingReloadsMutex;
    std::vector<PendingReload> m_PendingReloads;
    std::size_t m_ReloadTimestamp{ 0 };
    bool m_HasPendingReloads{ false };
};
