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
    std::vector<VfsMount*> LinkedMounts;
    std::unique_ptr<IVfsMountImpl> MountImpl;
};

VirtualFilesystem::VirtualFilesystem()
{
    srand(static_cast<unsigned int>(time(nullptr))); // use something better for randomness?
}
VirtualFilesystem::~VirtualFilesystem() = default;

VirtualFilesystem::VfsMount* VirtualFilesystem::MountFolder(std::string_view path, std::int64_t priority, VfsType type)
{
    namespace fs = std::filesystem;

    LogInfo("Mounting folder '{}' as a virtual filesystem...", path);

    auto it = std::upper_bound(mMounts.begin(), mMounts.end(), priority, [](std::int64_t prio, const auto& mount)
                               { return mount->Priority > prio; });
    VfsMount* new_mount = new VfsMount{ .Priority = priority, .MountImpl = std::make_unique<VfsFolderMount>(path, type) };
    mMounts.emplace(it, new_mount);

    return new_mount;
}
void VirtualFilesystem::LinkMounts(struct VfsMount* lhs_mount, struct VfsMount* rhs_mount)
{
    if (!algo::contains(lhs_mount->LinkedMounts, lhs_mount))
    {
        lhs_mount->LinkedMounts.push_back(lhs_mount);
    }
    if (!algo::contains(rhs_mount->LinkedMounts, rhs_mount))
    {
        rhs_mount->LinkedMounts.push_back(rhs_mount);
    }
    if (!algo::contains(lhs_mount->LinkedMounts, rhs_mount))
    {
        lhs_mount->LinkedMounts.push_back(rhs_mount);
        rhs_mount->LinkedMounts.push_back(lhs_mount);
        auto index_sort_pred = [this](const VfsMount* lhs, const VfsMount* rhs) {
            size_t lhs_index = algo::find(mMounts, &std::unique_ptr<VfsMount>::get, lhs) - &mMounts.front();
            size_t rhs_index = algo::find(mMounts, &std::unique_ptr<VfsMount>::get, rhs) - &mMounts.front();
            return lhs_index < rhs_index;
        };
        algo::sort(lhs_mount->LinkedMounts, index_sort_pred);
        algo::sort(rhs_mount->LinkedMounts, index_sort_pred);
    }
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

void VirtualFilesystem::LinkPathes(std::vector<LinkedPathesElement> pathes)
{
    if (LinkedPathes* linked_pathes = GetLinkedPathes(pathes))
    {
        for (LinkedPathesElement& path : pathes)
        {
            if (!algo::contains(*linked_pathes, &LinkedPathesElement::Path, path.Path))
            {
                linked_pathes->push_back(std::move(path));
            }
        }
    }
    else
    {
        m_LinkedPathes.push_back(std::move(pathes));
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
    // Same reasoning for linked pathes
    for (const auto& mount : mMounts)
    {
        if (!m_CustomFilters.empty())
        {
            if (const auto& asset_path = mount->MountImpl->GetFilePath(path))
            {
                if (!FilterPath(asset_path.value(), path_view, {}))
                {
                    continue;
                }
            }
        }

        if (FileInfo* loaded_data = mount->MountImpl->LoadFile(path, allocator))
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

    if (const VfsMount* linked_mount = GetLinkedMount(path, path_view, allowed_extensions, type))
    {
        return GetFilePath(linked_mount, path);
    }

    if (const VfsMount* loading_mount = GetLoadingMount(path, path_view, allowed_extensions, type))
    {
        return GetFilePath(loading_mount, path);
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

    if (const VfsMount* linked_mount = GetRandomLinkedMount(path, path_view, allowed_extensions, type))
    {
        return GetFilePath(linked_mount, path);
    }

    if (const VfsMount* loading_mount = GetRandomLoadingMount(path, path_view, allowed_extensions, type))
    {
        return GetFilePath(loading_mount, path);
    }

    return std::nullopt;
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

    for (const auto& mount : mMounts)
    {
        if (mount->MountImpl->IsType(type))
        {
            if (auto file_path = mount->MountImpl->GetFilePath(path))
            {
                file_paths.push_back(std::move(file_path).value());
            }
        }
    }

    return file_paths;
}

std::optional<std::filesystem::path> VirtualFilesystem::GetFilePath(const VfsMount* mount, const std::filesystem::path& path) const
{
    if (mount->LinkedMounts.empty())
    {
        return mount->MountImpl->GetFilePath(path);
    }
    else
    {
        for (const VfsMount* linked_mount : mount->LinkedMounts)
        {
            if (const auto return_path = linked_mount->MountImpl->GetFilePath(path))
            {
                return return_path;
            }
        }
    }

    return std::nullopt;
}

const VirtualFilesystem::VfsMount* VirtualFilesystem::GetLinkedMount(
    [[maybe_unused]] const std::filesystem::path& path,
    std::string_view path_view,
    [[maybe_unused]] std::span<const std::filesystem::path> allowed_extensions,
    VfsType type) const
{
    if (const LinkedPathes* linked_pathes = GetLinkedPathes(path_view))
    {
#ifdef _DEBUG
        for (const LinkedPathesElement& linked_path : *linked_pathes)
        {
            if (path == linked_path.Path)
            {
                std::vector<std::filesystem::path> lhs{ allowed_extensions.begin(), allowed_extensions.end() };
                std::vector<std::filesystem::path> rhs{ linked_path.AllowedExtensions.begin(), linked_path.AllowedExtensions.end() };
                assert(lhs == rhs);
            }
        }
#endif

        const CachedMountKey cache_key{ linked_pathes };

        std::lock_guard lock{ m_MountCacheMutex };
        if (const CachedMount* cached_mount = algo::find(m_MountCache, &CachedMount::Key, cache_key))
        {
            return cached_mount->Mount;
        }

        const VfsMount* linked_mount{ nullptr };
        for (const LinkedPathesElement& linked_path : *linked_pathes)
        {
            const std::string linked_path_string{ linked_path.Path.string() };
            const std::string_view linked_path_view{ linked_path_string };
            if (const VfsMount* loading_mount = GetLoadingMount(linked_path.Path, linked_path_view, linked_path.AllowedExtensions, type))
            {
                if (linked_mount == nullptr || loading_mount->Priority < linked_mount->Priority)
                {
                    linked_mount = loading_mount;
                }
            }
        }

        m_MountCache.push_back(CachedMount{ cache_key, linked_mount });
        return linked_mount;
    }

    return nullptr;
}
const VirtualFilesystem::VfsMount* VirtualFilesystem::GetLoadingMount(
    const std::filesystem::path& path,
    std::string_view path_view,
    std::span<const std::filesystem::path> allowed_extensions,
    VfsType type) const
{
    if (const BoundPathes* bound_pathes = GetBoundPathes(path_view))
    {
        std::int64_t current_file_prio{ std::numeric_limits<std::int64_t>::max() };
        const VfsMount* file_mount{ nullptr };

        for (std::string_view bound_path_str : *bound_pathes)
        {
            std::filesystem::path bound_path = bound_path_str;
            for (const auto& mount : mMounts)
            {
                if (mount->MountImpl->IsType(type) && mount->Priority < current_file_prio)
                {
                    if (auto bound_file_path = mount->MountImpl->GetFilePath(bound_path))
                    {
                        if (!FilterPath(bound_file_path.value(), path_view, allowed_extensions))
                        {
                            continue;
                        }

                        current_file_prio = mount->Priority;

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
                            file_mount = mount.get();
                        }
                        else
                        {
                            file_mount = nullptr;
                        }

                        break; // into loop over bound pathes
                    }
                }
            }

            // No need to continue looking if we found a file in the first mount
            if (current_file_prio == mMounts.front()->Priority)
            {
                break;
            }
        }

        return file_mount;
    }
    else
    {
        for (const auto& mount : mMounts)
        {
            if (mount->MountImpl->IsType(type))
            {
                if (auto file_path = mount->MountImpl->GetFilePath(path))
                {
                    if (FilterPath(file_path.value(), path_view, allowed_extensions))
                    {
                        return mount.get();
                    }
                }
            }
        }
    }

    return nullptr;
}

const VirtualFilesystem::VfsMount* VirtualFilesystem::GetRandomLinkedMount(
    [[maybe_unused]] const std::filesystem::path& path,
    std::string_view path_view,
    [[maybe_unused]] std::span<const std::filesystem::path> allowed_extensions,
    VfsType type) const
{
    if (const LinkedPathes* linked_pathes = GetLinkedPathes(path_view))
    {
#ifdef _DEBUG
        for (const LinkedPathesElement& linked_path : *linked_pathes)
        {
            if (path == linked_path.Path)
            {
                std::vector<std::filesystem::path> lhs{ allowed_extensions.begin(), allowed_extensions.end() };
                std::vector<std::filesystem::path> rhs{ linked_path.AllowedExtensions.begin(), linked_path.AllowedExtensions.end() };
                assert(lhs == rhs);
            }
        }
#endif
        const CachedMountKey cache_key{ linked_pathes };

        std::lock_guard lock{ m_MountCacheMutex };
        if (const CachedMount* cached_mount = algo::find(m_MountCache, &CachedMount::Key, cache_key))
        {
            return cached_mount->Mount;
        }

        const VfsMount* linked_mount{ [&]() -> const VfsMount*
                                      {
                                          std::vector<const VfsMount*> all_mounts;
                                          for (const LinkedPathesElement& linked_path : *linked_pathes)
                                          {
                                              const std::string linked_path_string{ linked_path.Path.string() };
                                              const std::string_view linked_path_view{ linked_path_string };
                                              std::vector<const VfsMount*> mounts = GetAllLoadingMounts(linked_path.Path, linked_path_view, linked_path.AllowedExtensions, type);
                                              all_mounts.insert(all_mounts.end(), mounts.begin(), mounts.end());
                                          }
                                          std::sort(all_mounts.begin(), all_mounts.end());
                                          all_mounts.erase(std::unique(all_mounts.begin(), all_mounts.end()), all_mounts.end());
                                          if (!all_mounts.empty())
                                          {
                                              return all_mounts[rand() % all_mounts.size()]; // use something better for randomness??? nah...
                                          }
                                          else
                                          {
                                              return nullptr;
                                          }
                                      }() };

        // Also cache for all bound-pathes of all linked files in case a path is bound to a path but not linked to it
        std::vector<const BoundPathes*> all_bound_pathes;
        for (const LinkedPathesElement& linked_path : *linked_pathes)
        {
            const std::string linked_path_string{ linked_path.Path.string() };
            const std::string_view linked_path_view{ linked_path_string };
            if (const BoundPathes* bound_pathes = GetBoundPathes(linked_path_view))
            {
                all_bound_pathes.push_back(bound_pathes);
            }
        }
        std::sort(all_bound_pathes.begin(), all_bound_pathes.end());
        all_bound_pathes.erase(std::unique(all_bound_pathes.begin(), all_bound_pathes.end()), all_bound_pathes.end());

        m_MountCache.push_back(CachedMount{ cache_key, linked_mount });
        for (const BoundPathes* bound_pathes : all_bound_pathes)
        {
            m_MountCache.push_back(CachedMount{ CachedMountKey{ bound_pathes }, linked_mount });
        }

        return linked_mount;
    }

    return nullptr;
}
const VirtualFilesystem::VfsMount* VirtualFilesystem::GetRandomLoadingMount(
    const std::filesystem::path& path,
    std::string_view path_view,
    std::span<const std::filesystem::path> allowed_extensions,
    VfsType type) const
{
    CachedMountKey cache_key = [&]()
    {
        if (const BoundPathes* bound_pathes = GetBoundPathes(path_view))
        {
            return CachedMountKey{ bound_pathes };
        }
        else
        {
            return CachedMountKey{ path };
        }
    }();

    std::lock_guard lock{ m_MountCacheMutex };
    const CachedMount* cached_mount = algo::find(m_MountCache, &CachedMount::Key, cache_key);
    if (cached_mount == nullptr)
    {
        const VfsMount* selected_mount{ [&]() -> const VfsMount*
                                        {
                                            std::vector<const VfsMount*> mounts = GetAllLoadingMounts(path, path_view, allowed_extensions, type);
                                            if (!mounts.empty())
                                            {
                                                return mounts[rand() % mounts.size()]; // use something better for randomness??? nah...
                                            }
                                            else
                                            {
                                                return nullptr;
                                            }
                                        }() };
        m_MountCache.push_back(CachedMount{ cache_key, selected_mount });
        cached_mount = &m_MountCache.back();
    }

    return cached_mount->Mount;
}

std::vector<const VirtualFilesystem::VfsMount*> VirtualFilesystem::GetAllLoadingMounts(
    const std::filesystem::path& path,
    std::string_view path_view,
    std::span<const std::filesystem::path> allowed_extensions,
    VfsType type) const
{
    std::vector<const VfsMount*> mounts;
    if (const BoundPathes* bound_pathes = GetBoundPathes(path_view))
    {
        for (std::string_view bound_path : *bound_pathes)
        {
            for (const auto& mount : mMounts)
            {
                if (mount->MountImpl->IsType(type))
                {
                    if (auto file_path_res = mount->MountImpl->GetFilePath(bound_path))
                    {
                        std::filesystem::path file_path = std::move(file_path_res).value();
                        if (FilterPath(file_path, path_view, allowed_extensions))
                        {
                            mounts.push_back(mount.get());
                        }
                    }
                }
            }
        }
    }
    else
    {
        for (const auto& mount : mMounts)
        {
            if (mount->MountImpl->IsType(type))
            {
                if (auto file_path_res = mount->MountImpl->GetFilePath(path))
                {
                    std::filesystem::path file_path = std::move(file_path_res).value();
                    if (FilterPath(file_path, path_view, allowed_extensions))
                    {
                        mounts.push_back(mount.get());
                    }
                }
            }
        }
    }
    return mounts;
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

template<class PathListSourceT>
auto* GetBoundPathesImpl(std::string_view path, PathListSourceT& source)
{
    path = path.substr(0, path.rfind('.'));
    return algo::find_if(source,
                         [path](const std::vector<std::string_view>& bound_pathes)
                         {
                             return algo::contains(bound_pathes, path);
                         });
}
template<class PathListSourceT>
auto* GetBoundPathesImpl(const std::vector<std::string_view>& pathes, PathListSourceT& source)
{
    return algo::find_if(source,
                         [&pathes](const std::vector<std::string_view>& bound_pathes)
                         {
                             return algo::contains_if(bound_pathes,
                                                      [&pathes](std::string_view bound_path)
                                                      {
                                                          return algo::contains(pathes, bound_path);
                                                      });
                         });
}

VirtualFilesystem::BoundPathes* VirtualFilesystem::GetBoundPathes(std::string_view path)
{
    return GetBoundPathesImpl(path, m_BoundPathes);
}
VirtualFilesystem::BoundPathes* VirtualFilesystem::GetBoundPathes(const BoundPathes& pathes)
{
    return GetBoundPathesImpl(pathes, m_BoundPathes);
}
const VirtualFilesystem::BoundPathes* VirtualFilesystem::GetBoundPathes(std::string_view path) const
{
    return GetBoundPathesImpl(path, m_BoundPathes);
}
const VirtualFilesystem::BoundPathes* VirtualFilesystem::GetBoundPathes(const BoundPathes& pathes) const
{
    return GetBoundPathesImpl(pathes, m_BoundPathes);
}

template<class PathListSourceT>
auto* GetLinkedPathesImpl(std::string_view path, PathListSourceT& source)
{
    path = path.substr(0, path.rfind('.'));
    return algo::find_if(source,
                         [path](const std::vector<VirtualFilesystem::LinkedPathesElement>& linked_pathes)
                         {
                             return algo::contains(linked_pathes, &VirtualFilesystem::LinkedPathesElement::Path, path);
                         });
}
template<class PathListSourceT>
auto* GetLinkedPathesImpl(const std::vector<VirtualFilesystem::LinkedPathesElement>& pathes, PathListSourceT& source)
{
    return algo::find_if(source,
                         [&pathes](const std::vector<VirtualFilesystem::LinkedPathesElement>& linked_pathes)
                         {
                             return algo::contains_if(linked_pathes,
                                                      [&pathes](const VirtualFilesystem::LinkedPathesElement& linked_path)
                                                      {
                                                          return algo::contains(pathes, &VirtualFilesystem::LinkedPathesElement::Path, linked_path.Path);
                                                      });
                         });
}

VirtualFilesystem::LinkedPathes* VirtualFilesystem::GetLinkedPathes(std::string_view path)
{
    return GetLinkedPathesImpl(path, m_LinkedPathes);
}
VirtualFilesystem::LinkedPathes* VirtualFilesystem::GetLinkedPathes(const LinkedPathes& pathes)
{
    return GetLinkedPathesImpl(pathes, m_LinkedPathes);
}
const VirtualFilesystem::LinkedPathes* VirtualFilesystem::GetLinkedPathes(std::string_view path) const
{
    return GetLinkedPathesImpl(path, m_LinkedPathes);
}
const VirtualFilesystem::LinkedPathes* VirtualFilesystem::GetLinkedPathes(const LinkedPathes& pathes) const
{
    return GetLinkedPathesImpl(pathes, m_LinkedPathes);
}
