#include "virtual_filesystem.h"

#include "log.h"
#include "util/algorithms.h"
#include "util/on_scope_exit.h"

#include <spel2.h>

#include <Windows.h>
#include <filesystem>

class IVfsMountImpl
{
  public:
    virtual ~IVfsMountImpl() = default;

    using FileInfo = VirtualFilesystem::FileInfo;
    virtual FileInfo* LoadFile(const char* file_path, void* (*allocator)(std::size_t)) const = 0;
    virtual std::optional<std::filesystem::path> GetFilePath(const std::filesystem::path& path) const = 0;
    virtual bool IsType(VfsType type) const = 0;
};

class VfsFolderMount : public IVfsMountImpl
{
  public:
    VfsFolderMount(std::filesystem::path mounted_path, VfsType type)
        : mMountedPath(std::move(mounted_path))
        , mMountedPathString(mMountedPath.string())
        , mType(type)
    {
        std::replace(mMountedPathString.begin(), mMountedPathString.end(), '\\', '/');
    }
    virtual ~VfsFolderMount() override = default;

    virtual FileInfo* LoadFile(const char* file_path, void* (*allocator)(std::size_t)) const override
    {
        char full_path[MAX_PATH];
        if (mMountedPathString.empty())
        {
            strcpy_s(full_path, file_path);
        }
        else
        {
            sprintf_s(full_path, "%s/%s", mMountedPathString.c_str(), file_path);
        }

        FILE* file{ nullptr };
        auto error = fopen_s(&file, full_path, "rb");
        if (error == 0 && file != nullptr)
        {
            auto close_file = OnScopeExit{ [file]()
                                           { fclose(file); } };

            fseek(file, 0, SEEK_END);
            const std::size_t file_size = ftell(file);
            fseek(file, 0, SEEK_SET);

            if (allocator == nullptr)
            {
                allocator = malloc;
            }

            const std::size_t allocation_size = file_size + sizeof(FileInfo);
            if (void* buf = allocator(allocation_size))
            {
                void* data = static_cast<void*>(reinterpret_cast<char*>(buf) + 24);
                const auto size_read = fread(data, 1, file_size, file);
                if (size_read != file_size)
                {
                    LogError("Could not read file {}, this will either crash or cause glitches...", full_path);
                }

                FileInfo* file_info = new (buf) FileInfo();
                *file_info = {
                    .Data = data,
                    .DataSize = static_cast<int>(file_size),
                    .AllocationSize = static_cast<int>(allocation_size)
                };

                return file_info;
            }
        }

        return nullptr;
    }

    virtual std::optional<std::filesystem::path> GetFilePath(const std::filesystem::path& path) const override
    {
        namespace fs = std::filesystem;
        if (path.has_extension())
        {
            const auto full_path = mMountedPath / path;
            if (fs::exists(full_path))
            {
                return std::move(full_path);
            }
        }
        else
        {
            // Painfully check each file if it matches the name, return first match
            auto parent_path = path.has_parent_path() ? mMountedPath / path.parent_path() : mMountedPath;
            if (fs::exists(parent_path))
            {
                auto file_name = path.filename();
                for (auto& dir_path : fs::directory_iterator{ parent_path })
                {
                    if (fs::is_regular_file(dir_path) && algo::is_same_path(dir_path.path().stem(), file_name))
                    {
                        return dir_path.path();
                    }
                }
            }
        }

        return std::nullopt;
    }

    virtual bool IsType(VfsType type) const override
    {
        return type == VfsType::Any || type == mType;
    }

  private:
    std::filesystem::path mMountedPath;
    std::string mMountedPathString;
    VfsType mType;
};

struct VirtualFilesystem::VfsMount
{
    std::int64_t Priority;
    std::unique_ptr<IVfsMountImpl> MountImpl;
};

VirtualFilesystem::VirtualFilesystem()
{
    srand(static_cast<unsigned int>(time(nullptr))); // use something better for randomness?
}
VirtualFilesystem::~VirtualFilesystem() = default;

void VirtualFilesystem::MountFolder(std::string_view path, std::int64_t priority, VfsType type)
{
    namespace fs = std::filesystem;

    LogInfo("Mounting folder '{}' as a virtual filesystem...", path);

    auto it = std::upper_bound(mMounts.begin(), mMounts.end(), priority, [](std::int64_t prio, const VfsMount& mount)
                               { return mount.Priority > prio; });
    mMounts.insert(it, VfsMount{ .Priority = priority, .MountImpl = std::make_unique<VfsFolderMount>(path, type) });
}

void VirtualFilesystem::RestrictFiles(std::span<const std::string_view> files)
{
    m_RestrictedFiles = files;
}

void VirtualFilesystem::RegisterCustomFilter(CustomFilterFun filter)
{
    m_CustomFilters.push_back(std::move(filter));
}

void VirtualFilesystem::BindPathes(std::vector<std::string_view> pathes)
{
    if (std::vector<std::string_view>* bound_pathes = GetBoundPathes(pathes))
    {
        for (std::string_view path : pathes)
        {
            if (!algo::contains(*bound_pathes, path))
            {
                bound_pathes->push_back(path);
            }
        }
    }
    else
    {
        m_BoundPathes.push_back(std::move(pathes));
    }
}

VirtualFilesystem::FileInfo* VirtualFilesystem::LoadFile(const char* path, void* (*allocator)(std::size_t)) const
{
    if (mMounts.empty())
    {
        return nullptr;
    }

    std::string_view path_view{ path };

    if (!m_RestrictedFiles.empty())
    {
        std::string_view path_no_extension{ path_view };
        path_no_extension = path_no_extension.substr(0, path_no_extension.rfind('.'));
        if (!algo::contains(m_RestrictedFiles, path_no_extension))
        {
            return nullptr;
        }
    }

    // Should not need to use bound pathes here because those should all be handled during preprocessing
    // Bound pathes should usually contain one 'actual' game asset and the rest addon assets
    for (const VfsMount& mount : mMounts)
    {
        if (!m_CustomFilters.empty())
        {
            if (const auto& asset_path = mount.MountImpl->GetFilePath(path))
            {
                if (!FilterPath(asset_path.value(), path_view, {}))
                {
                    continue;
                }
            }
        }

        if (FileInfo* loaded_data = mount.MountImpl->LoadFile(path, allocator))
        {
            return loaded_data;
        }
    }

    return nullptr;
}

std::optional<std::filesystem::path> VirtualFilesystem::GetFilePath(const std::filesystem::path& path, VfsType type) const
{
    return GetFilePathFilterExt(path, {}, type);
}
std::optional<std::filesystem::path> VirtualFilesystem::GetFilePathFilterExt(const std::filesystem::path& path, std::span<const std::filesystem::path> allowed_extensions, VfsType type) const
{
    if (mMounts.empty())
    {
        return std::nullopt;
    }

    const std::string path_string = path.string();
    std::string_view path_view{ path_string };

    if (!m_RestrictedFiles.empty())
    {
        std::string_view path_no_extension{ path_view };
        path_no_extension = path_no_extension.substr(0, path_no_extension.rfind('.'));
        if (!algo::contains(m_RestrictedFiles, path_no_extension))
        {
            return std::nullopt;
        }
    }

    if (const BoundPathes* bound_pathes = GetBoundPathes(path_string))
    {
        std::int64_t current_file_prio{ std::numeric_limits<std::int64_t>::max() };
        std::optional<std::filesystem::path> file_path{ std::nullopt };

        for (std::string_view bound_path_str : *bound_pathes)
        {
            std::filesystem::path bound_path = bound_path_str;
            for (const VfsMount& mount : mMounts)
            {
                if (mount.MountImpl->IsType(type) && mount.Priority < current_file_prio)
                {
                    if (auto bound_file_path = mount.MountImpl->GetFilePath(bound_path)) // TODO: needs to ignore extension
                    {
                        if (!FilterPath(bound_file_path.value(), path_view, allowed_extensions))
                        {
                            continue;
                        }

                        current_file_prio = mount.Priority;

                        auto is_this_path = [&path](const std::filesystem::path& found_path)
                        {
                            if (path.has_extension())
                            {
                                return algo::is_end_of_path(path, found_path);
                            }
                            else
                            {
                                auto found_path_no_ext = std::filesystem::path{ found_path }.replace_extension();
                                return algo::is_end_of_path(path, found_path_no_ext);
                            }
                        };

                        // Only assign pathes that we are actually looking for
                        // Anything else blocks lower prio files by design
                        if (is_this_path(bound_file_path.value()))
                        {
                            file_path = std::move(bound_file_path).value();
                        }
                        else
                        {
                            file_path.reset();
                        }

                        break; // into loop over bound pathes
                    }
                }
            }

            // No need to continue looking if we found a file in the first mount
            if (current_file_prio == mMounts.front().Priority)
            {
                break;
            }
        }

        return file_path;
    }
    else
    {
        for (const VfsMount& mount : mMounts)
        {
            if (mount.MountImpl->IsType(type))
            {
                if (auto file_path = mount.MountImpl->GetFilePath(path))
                {
                    if (FilterPath(file_path.value(), path_view, allowed_extensions))
                    {
                        return file_path;
                    }
                }
            }
        }
    }

    return std::nullopt;
}
std::optional<std::filesystem::path> VirtualFilesystem::GetDifferentFilePath(const std::filesystem::path& path, VfsType type) const
{
    namespace fs = std::filesystem;
    for (const fs::path& found_path : GetAllFilePaths(path, type))
    {
        if (found_path.compare(path) != 0)
        {
            return found_path;
        }
    }
    return std::nullopt;
}
std::optional<std::filesystem::path> VirtualFilesystem::GetRandomFilePath(const std::filesystem::path& path, VfsType type) const
{
    return GetRandomFilePathFilterExt(path, {}, type);
}
std::optional<std::filesystem::path> VirtualFilesystem::GetRandomFilePathFilterExt(const std::filesystem::path& path, std::span<const std::filesystem::path> allowed_extensions, VfsType type) const
{
    if (!m_RestrictedFiles.empty())
    {
        const std::string path_no_extension{ std::filesystem::path{ path }.replace_extension().string() };
        if (!algo::contains(m_RestrictedFiles, path_no_extension))
        {
            return std::nullopt;
        }
    }

    const std::string path_string = path.string();
    std::string_view path_view{ path_string };

    if (const BoundPathes* bound_pathes = GetBoundPathes(path_string))
    {
        std::lock_guard lock{ m_RandomCacheMutex };
        const CachedRandomFile* cached_file = algo::find(m_RandomCache, &CachedRandomFile::TargetPath, CachedRandomFileKey{ bound_pathes });
        if (cached_file == nullptr)
        {
            std::vector<std::filesystem::path> file_paths;
            for (std::string_view bound_path : *bound_pathes)
            {
                for (const VfsMount& mount : mMounts)
                {
                    if (mount.MountImpl->IsType(type))
                    {
                        if (auto file_path = mount.MountImpl->GetFilePath(bound_path))
                        {
                            if (FilterPath(file_path.value(), path_view, allowed_extensions))
                            {
                                file_paths.push_back(std::move(file_path).value());
                            }
                        }
                    }
                }
            }
            if (!file_paths.empty())
            {
                m_RandomCache.push_back(CachedRandomFile{ CachedRandomFileKey{ bound_pathes }, file_paths[rand() % file_paths.size()] }); // use something better for randomness?
            }
            else
            {
                m_RandomCache.push_back(CachedRandomFile{ CachedRandomFileKey{ bound_pathes }, std::nullopt });
            }
            cached_file = &m_RandomCache.back();
        }

        if (cached_file->ResultPath.has_value() && algo::is_end_of_path(path, cached_file->ResultPath.value()))
        {
            return cached_file->ResultPath;
        }
        return std::nullopt;
    }
    else
    {

        std::lock_guard lock{ m_RandomCacheMutex };
        const CachedRandomFile* cached_file = algo::find(m_RandomCache, &CachedRandomFile::TargetPath, CachedRandomFileKey{ path });
        if (cached_file == nullptr)
        {
            std::vector<std::filesystem::path> file_paths;
            for (const VfsMount& mount : mMounts)
            {
                if (mount.MountImpl->IsType(type))
                {
                    if (auto file_path = mount.MountImpl->GetFilePath(path))
                    {
                        if (FilterPath(file_path.value(), path_view, allowed_extensions))
                        {
                            file_paths.push_back(std::move(file_path).value());
                        }
                    }
                }
            }

            if (!file_paths.empty())
            {
                m_RandomCache.push_back(CachedRandomFile{ CachedRandomFileKey{ path }, file_paths[rand() % file_paths.size()] }); // use something better for randomness?
            }
            else
            {
                m_RandomCache.push_back(CachedRandomFile{ CachedRandomFileKey{ path }, std::nullopt });
            }
            cached_file = &m_RandomCache.back();
        }

        return cached_file->ResultPath;
    }
}
std::vector<std::filesystem::path> VirtualFilesystem::GetAllFilePaths(const std::filesystem::path& path, VfsType type) const
{
    std::vector<std::filesystem::path> file_paths;

    if (!m_RestrictedFiles.empty())
    {
        const std::string path_no_extension{ std::filesystem::path{ path }.replace_extension().string() };
        if (!algo::contains(m_RestrictedFiles, path_no_extension))
        {
            return file_paths;
        }
    }

    for (const VfsMount& mount : mMounts)
    {
        if (mount.MountImpl->IsType(type))
        {
            if (auto file_path = mount.MountImpl->GetFilePath(path))
            {
                file_paths.push_back(std::move(file_path).value());
            }
        }
    }

    return file_paths;
}

bool VirtualFilesystem::FilterPath(const std::filesystem::path& path, std::string_view relative_path, std::span<const std::filesystem::path> allowed_extensions) const
{
    for (const auto& filter : m_CustomFilters)
    {
        if (!filter(path, relative_path))
        {
            return false;
        }
    }
    return allowed_extensions.empty() || algo::contains(allowed_extensions, path.extension());
}

VirtualFilesystem::BoundPathes* VirtualFilesystem::GetBoundPathes(std::string_view path)
{
    return const_cast<BoundPathes*>(static_cast<const VirtualFilesystem*>(this)->GetBoundPathes(path));
}
VirtualFilesystem::BoundPathes* VirtualFilesystem::GetBoundPathes(const BoundPathes& pathes)
{
    return const_cast<BoundPathes*>(static_cast<const VirtualFilesystem*>(this)->GetBoundPathes(pathes));
}
const VirtualFilesystem::BoundPathes* VirtualFilesystem::GetBoundPathes(std::string_view path) const
{
    path = path.substr(0, path.rfind('.'));
    return algo::find_if(m_BoundPathes,
                         [path](const std::vector<std::string_view>& bound_pathes)
                         {
                             return algo::contains(bound_pathes, path);
                         });
}
const VirtualFilesystem::BoundPathes* VirtualFilesystem::GetBoundPathes(const BoundPathes& pathes) const
{
    return algo::find_if(m_BoundPathes,
                         [&pathes](const std::vector<std::string_view>& bound_pathes)
                         {
                             return algo::contains_if(bound_pathes,
                                                      [&pathes](std::string_view bound_path)
                                                      {
                                                          return algo::contains(pathes, bound_path);
                                                      });
                         });
}
