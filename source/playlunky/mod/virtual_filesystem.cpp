#include "virtual_filesystem.h"

#include "log.h"
#include "util/algorithms.h"
#include "util/on_scope_exit.h"

#include <Windows.h>
#include <filesystem>

class IVfsMountImpl {
public:
	virtual ~IVfsMountImpl() = default;

	using FileInfo = VirtualFilesystem::FileInfo;
	virtual FileInfo* LoadFile(const char* file_path, void* (*allocator)(std::size_t)) const = 0;
	virtual std::optional<std::filesystem::path> GetFilePath(const std::filesystem::path& path) const = 0;
};

class VfsFolderMount : public IVfsMountImpl {
public:
	VfsFolderMount(std::filesystem::path mounted_path)
		: mMountedPath(std::move(mounted_path))
		, mMountedPathString(mMountedPath.string()) {
		std::replace(mMountedPathString.begin(), mMountedPathString.end(), '\\', '/');
	}
	virtual ~VfsFolderMount() override = default;

	virtual FileInfo* LoadFile(const char* file_path, void* (*allocator)(std::size_t)) const override {
		char full_path[MAX_PATH];
		sprintf_s(full_path, "%s/%s", mMountedPathString.c_str(), file_path);

		FILE* file{ nullptr };
		auto error = fopen_s(&file, full_path, "rb");
		if (error == 0 && file != nullptr) {
			auto close_file = OnScopeExit{ [file]() { fclose(file); } };

			fseek(file, 0, SEEK_END);
			const std::size_t file_size = ftell(file);
			fseek(file, 0, SEEK_SET);

			if (allocator == nullptr) {
				allocator = malloc;
			}

			const std::size_t allocation_size = file_size + sizeof(FileInfo);
			if (void* buf = allocator(allocation_size)) {
				void* data = static_cast<void*>(reinterpret_cast<char*>(buf) + 24);
				const auto size_read = fread(data, 1, file_size, file);
				if (size_read != file_size) {
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

	virtual std::optional<std::filesystem::path> GetFilePath(const std::filesystem::path& path) const override {
		std::filesystem::path full_path = mMountedPath / path;
		if (std::filesystem::exists(full_path)) {
			return std::move(full_path);
		}
		return std::nullopt;
	}

private:
	std::filesystem::path mMountedPath;
	std::string mMountedPathString;
};

struct VirtualFilesystem::VfsMount {
	std::int64_t Priority;
	std::unique_ptr<IVfsMountImpl> MountImpl;
};

VirtualFilesystem::VirtualFilesystem() {
	srand(static_cast<unsigned int>(time(nullptr))); // use something better for randomness?
}
VirtualFilesystem::~VirtualFilesystem() = default;

void VirtualFilesystem::MountFolder(std::string_view path, std::int64_t priority) {
	namespace fs = std::filesystem;

	LogInfo("Mounting folder '{}' as a virtual filesystem...", path);

	auto it = std::upper_bound(mMounts.begin(), mMounts.end(), priority, [](std::int64_t prio, const VfsMount& mount) { return mount.Priority > prio; });
	mMounts.insert(it, VfsMount{
			.Priority = priority,
			.MountImpl = std::make_unique<VfsFolderMount>(path)
		});
}
void VirtualFilesystem::BindPathes(std::vector<std::string_view> pathes) {
	if (std::vector<std::string_view>* bound_pathes = GetBoundPathes(pathes)) {
		for (std::string_view path : pathes) {
			if (!algo::contains(*bound_pathes, path)) {
				bound_pathes->push_back(path);
			}
		}
	}
	else {
		m_BoundPathes.push_back(std::move(pathes));
	}
}

VirtualFilesystem::FileInfo* VirtualFilesystem::LoadFile(const char* path, void* (*allocator)(std::size_t)) const {
	// Should not need to use bound pathes here because those should all be handled during preprocessing
	// Bound pathes should usually contain one 'actual' game asset and the rest addon assets
	for (const VfsMount& mount : mMounts) {
		if (FileInfo* loaded_data = mount.MountImpl->LoadFile(path, allocator)) {
			return loaded_data;
		}
	}

	return nullptr;
}

std::optional<std::filesystem::path> VirtualFilesystem::GetFilePath(const std::filesystem::path& path) const {
	if (const BoundPathes* bound_pathes = GetBoundPathes(path.string())) {
		std::int64_t current_file_prio{ std::numeric_limits<std::int64_t>::max() };
		std::optional<std::filesystem::path> file_path{ std::nullopt };

		for (std::string_view bound_path : *bound_pathes) {
			for (const VfsMount& mount : mMounts) {
				if (mount.Priority < current_file_prio) {
					if (auto bound_file_path = mount.MountImpl->GetFilePath(bound_path)) {
						current_file_prio = mount.Priority;

						// Only assign pathes that we are actually looking for
						// Anything else blocks lower prio files by design
						if (algo::is_end_of_path(path, *bound_file_path)) {
							file_path = std::move(bound_file_path).value();
						}
						else {
							file_path.reset();
						}

						break; // into loop over bound pathes
					}
				}
			}

			// No need to continue looking if we found a file in the first mount
			if (current_file_prio == mMounts.front().Priority) {
				break;
			}
		}

		return file_path;
	}
	else {
		for (const VfsMount& mount : mMounts) {
			if (auto file_path = mount.MountImpl->GetFilePath(path)) {
				return file_path;
			}
		}
	}

	return std::nullopt;
}
std::optional<std::filesystem::path> VirtualFilesystem::GetRandomFilePath(const std::filesystem::path& path) const {
	if (const BoundPathes* bound_pathes = GetBoundPathes(path.string())) {

		std::lock_guard lock{ m_RandomCacheMutex };
		const CachedRandomFile* cached_file = algo::find_if(m_RandomCache, [bound_pathes](const CachedRandomFile& cached_file) {
			return cached_file.TargetPath == CachedRandomFileKey{ bound_pathes };
		});
		if (cached_file == nullptr) {
			std::vector<std::filesystem::path> file_paths;
			for (std::string_view bound_path : *bound_pathes) {
				for (const VfsMount& mount : mMounts) {
					if (auto file_path = mount.MountImpl->GetFilePath(bound_path)) {
						file_paths.push_back(std::move(file_path).value());
					}
				}
			}
			if (!file_paths.empty()) {
				m_RandomCache.push_back(CachedRandomFile{ CachedRandomFileKey{ bound_pathes }, file_paths[rand() % file_paths.size()] }); // use something better for randomness?
			}
			else {
				m_RandomCache.push_back(CachedRandomFile{ CachedRandomFileKey{ bound_pathes }, std::nullopt });
			}
			cached_file = &m_RandomCache.back();
		}

		if (cached_file->ResultPath.has_value() && algo::is_end_of_path(path, cached_file->ResultPath.value())) {
			return cached_file->ResultPath;
		}
		return std::nullopt;
	}
	else {

		std::lock_guard lock{ m_RandomCacheMutex };
		const CachedRandomFile* cached_file = algo::find_if(m_RandomCache, [&path](const CachedRandomFile& cached_file) {
			return cached_file.TargetPath == CachedRandomFileKey{ path };
		});
		if (cached_file == nullptr) {
			std::vector<std::filesystem::path> file_paths;
			for (const VfsMount& mount : mMounts) {
				if (auto file_path = mount.MountImpl->GetFilePath(path)) {
					file_paths.push_back(std::move(file_path).value());
				}
			}

			if (!file_paths.empty()) {
				m_RandomCache.push_back(CachedRandomFile{ CachedRandomFileKey{ path }, file_paths[rand() % file_paths.size()] }); // use something better for randomness?
			}
			else {
				m_RandomCache.push_back(CachedRandomFile{ CachedRandomFileKey{ path }, std::nullopt });
			}
			cached_file = &m_RandomCache.back();
		}

		return cached_file->ResultPath;
	}
}
std::vector<std::filesystem::path> VirtualFilesystem::GetAllFilePaths(const std::filesystem::path& path) const {
	std::vector<std::filesystem::path> file_paths;

	for (const VfsMount& mount : mMounts) {
		if (auto file_path = mount.MountImpl->GetFilePath(path)) {
			file_paths.push_back(std::move(file_path).value());
		}
	}

	return file_paths;
}

VirtualFilesystem::BoundPathes* VirtualFilesystem::GetBoundPathes(std::string_view path) {
	return const_cast<BoundPathes*>(static_cast<const VirtualFilesystem*>(this)->GetBoundPathes(path));
}
VirtualFilesystem::BoundPathes* VirtualFilesystem::GetBoundPathes(const BoundPathes& pathes) {
	return const_cast<BoundPathes*>(static_cast<const VirtualFilesystem*>(this)->GetBoundPathes(pathes));

}
const VirtualFilesystem::BoundPathes* VirtualFilesystem::GetBoundPathes(std::string_view path) const {
	return algo::find_if(m_BoundPathes,
		[path](const std::vector<std::string_view>& bound_pathes) {
		return algo::contains(bound_pathes, path);
	});
}
const VirtualFilesystem::BoundPathes* VirtualFilesystem::GetBoundPathes(const BoundPathes& pathes) const {
	return algo::find_if(m_BoundPathes,
		[&pathes](const std::vector<std::string_view>& bound_pathes) {
		return algo::contains_if(bound_pathes,
			[&pathes](std::string_view bound_path) {
				return algo::contains(pathes, bound_path);
			});
	});
}
