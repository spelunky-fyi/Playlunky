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

enum class VfsType
{
    Any,
    Backend,
    User,
};

class VirtualFilesystem
{
  public:
    VirtualFilesystem();
    ~VirtualFilesystem();

    VirtualFilesystem(const VirtualFilesystem&) = delete;
    VirtualFilesystem(VirtualFilesystem&&) = delete;
    VirtualFilesystem& operator=(const VirtualFilesystem&) = delete;
    VirtualFilesystem& operator=(VirtualFilesystem&&) = delete;

    struct VfsMount;
    VfsMount* MountFolder(std::string_view path, std::int64_t priority, VfsType vfs_type);
    void LinkMounts(struct VfsMount* lhs, struct VfsMount* rhs);

    // Allow loading only files specified in this list
    void RestrictFiles(std::span<const std::string_view> files);
    bool HasRestrictedFiles() const
    {
        return !m_RestrictedFiles.empty();
    }

    // Register a filter to block loading arbitrary files, return true from the filter to allow loading
    using CustomFilterFun = std::function<bool(const std::filesystem::path&, std::string_view)>;
    void RegisterCustomFilter(CustomFilterFun filter);

    // Binding pathes makes sure that only one of the bound files can be loaded
    void BindPathes(std::vector<std::string_view> pathes);

    // Linking pathes makes sure that if a file is loaded from one mount all linked pathes will also be loaded
    // from that mount, or fail if not available on that mount
    struct LinkedPathesElement
    {
        std::filesystem::path Path;
        std::span<std::filesystem::path> AllowedExtensions;
    };
    void LinkPathes(std::vector<LinkedPathesElement> pathes);

    // Interface for runtime loading
    using FileInfo = SpelunkyFileInfo;
    FileInfo* LoadFile(const char* path, void* (*allocator)(std::size_t) = nullptr) const;

    // Interface for loading during preprocessing
    std::optional<std::filesystem::path> GetFilePath(const std::filesystem::path& path, VfsType type = VfsType::Any) const;
    std::optional<std::filesystem::path> GetFilePathFilterExt(const std::filesystem::path& path, std::span<const std::filesystem::path> allowed_extensions, VfsType type = VfsType::Any) const;
    std::optional<std::filesystem::path> GetDifferentFilePath(const std::filesystem::path& path, VfsType type = VfsType::Any) const;
    std::optional<std::filesystem::path> GetRandomFilePath(const std::filesystem::path& path, VfsType type = VfsType::Any) const;
    std::optional<std::filesystem::path> GetRandomFilePathFilterExt(const std::filesystem::path& path, std::span<const std::filesystem::path> allowed_extensions, VfsType type = VfsType::Any) const;
    std::vector<std::filesystem::path> GetAllFilePaths(const std::filesystem::path& path, VfsType type = VfsType::Any) const;

  private:
    using BoundPathes = std::vector<std::string_view>;
    using LinkedPathes = std::vector<LinkedPathesElement>;

    bool IsAllowedFile(const std::filesystem::path& path) const;

    std::optional<std::filesystem::path> GetFilePath(const VfsMount* mount, const std::filesystem::path& path, std::string_view path_view, std::span<const std::filesystem::path> allowed_extensions, VfsType type) const;

    const VfsMount* GetLinkedMount(const std::filesystem::path& path, std::string_view path_view, std::span<const std::filesystem::path> allowed_extensions, VfsType type) const;
    const VfsMount* GetLoadingMount(const std::filesystem::path& path, std::string_view path_view, std::span<const std::filesystem::path> allowed_extensions, VfsType type) const;

    const VfsMount* GetRandomLinkedMount(const std::filesystem::path& path, std::string_view path_view, std::span<const std::filesystem::path> allowed_extensions, VfsType type) const;
    const VfsMount* GetRandomLoadingMount(const std::filesystem::path& path, std::string_view path_view, std::span<const std::filesystem::path> allowed_extensions, VfsType type) const;

    std::vector<const VfsMount*> GetAllLoadingMounts(const std::filesystem::path& path, std::string_view path_view, std::span<const std::filesystem::path> allowed_extensions, VfsType type) const;

    bool FilterPath(const std::filesystem::path& path, std::string_view relative_path, std::span<const std::filesystem::path> allowed_extensions) const;

    BoundPathes* GetBoundPathes(std::string_view path);
    BoundPathes* GetBoundPathes(const BoundPathes& pathes);
    const BoundPathes* GetBoundPathes(std::string_view path) const;
    const BoundPathes* GetBoundPathes(const BoundPathes& pathes) const;

    LinkedPathes* GetLinkedPathes(std::string_view path);
    LinkedPathes* GetLinkedPathes(const LinkedPathes& pathes);
    const LinkedPathes* GetLinkedPathes(std::string_view path) const;
    const LinkedPathes* GetLinkedPathes(const LinkedPathes& pathes) const;

    std::vector<std::unique_ptr<VfsMount>> mMounts;

    using CachedMountKey = std::variant<const BoundPathes*, const LinkedPathes*, std::filesystem::path>;
    struct CachedMount
    {
        CachedMountKey Key;
        const VfsMount* Mount;
    };
    mutable std::mutex m_MountCacheMutex;
    mutable std::vector<CachedMount> m_MountCache;

    std::span<const std::string_view> m_RestrictedFiles;
    std::vector<CustomFilterFun> m_CustomFilters;

    std::vector<BoundPathes> m_BoundPathes;
    std::vector<LinkedPathes> m_LinkedPathes;
};
