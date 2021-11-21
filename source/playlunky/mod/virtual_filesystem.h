#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

struct SpelunkyFileInfo;

class VirtualFilesystem
{
  public:
    VirtualFilesystem();
    ~VirtualFilesystem();

    VirtualFilesystem(const VirtualFilesystem&) = delete;
    VirtualFilesystem(VirtualFilesystem&&) = delete;
    VirtualFilesystem& operator=(const VirtualFilesystem&) = delete;
    VirtualFilesystem& operator=(VirtualFilesystem&&) = delete;

    void MountFolder(std::string_view path, std::int64_t priority);

    // Allow loading only files specified in this list
    void RestrictFiles(std::span<const std::string_view> files);

    // Register a filter to block loading arbitrary files, return true from the filter to allow loading
    using CustomFilterFun = std::function<bool(const std::filesystem::path)>;
    void RegisterCustomFilter(CustomFilterFun filter);

    // Binding pathes makes sure that only one of the bound files can be loaded
    void BindPathes(std::vector<std::string_view> pathes);

    // Interface for runtime loading
    using FileInfo = SpelunkyFileInfo;
    FileInfo* LoadFile(const char* path, void* (*allocator)(std::size_t) = nullptr) const;

    // Interface for loading during preprocessing
    std::optional<std::filesystem::path> GetFilePath(const std::filesystem::path& path) const;
    std::optional<std::filesystem::path> GetDifferentFilePath(const std::filesystem::path& path) const;
    std::optional<std::filesystem::path> GetRandomFilePath(const std::filesystem::path& path) const;
    std::vector<std::filesystem::path> GetAllFilePaths(const std::filesystem::path& path) const;

  private:
    bool FilterPath(const std::filesystem::path& path) const;

    using BoundPathes = std::vector<std::string_view>;
    BoundPathes* GetBoundPathes(std::string_view path);
    BoundPathes* GetBoundPathes(const BoundPathes& pathes);
    const BoundPathes* GetBoundPathes(std::string_view path) const;
    const BoundPathes* GetBoundPathes(const BoundPathes& pathes) const;

    struct VfsMount;
    std::vector<VfsMount> mMounts;

    using CachedRandomFileKey = std::variant<const BoundPathes*, std::filesystem::path>;
    struct CachedRandomFile
    {
        CachedRandomFileKey TargetPath;
        std::optional<std::filesystem::path> ResultPath;
    };
    mutable std::mutex m_RandomCacheMutex;
    mutable std::vector<CachedRandomFile> m_RandomCache;

    std::span<const std::string_view> m_RestrictedFiles;
    std::vector<CustomFilterFun> m_CustomFilters;

    std::vector<BoundPathes> m_BoundPathes;
};
