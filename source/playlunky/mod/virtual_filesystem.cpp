#include "virtual_filesystem.h"

#include "util.h"
#include "../log.h"

#include <filesystem>

class IVfsMountImpl {
public:
	virtual ~IVfsMountImpl() = default;

	using FileInfo = VirtualFilesystem::FileInfo;
	virtual FileInfo* LoadFile(std::string_view file_path, void* (*allocator)(std::size_t)) const = 0;
};

class VfsFolderMount : public IVfsMountImpl {
public:
	VfsFolderMount(std::filesystem::path mounted_path)
		: mMountedPath(std::move(mounted_path)) {}
	virtual ~VfsFolderMount() override = default;

	virtual FileInfo* LoadFile(std::string_view file_path, void* (*allocator)(std::size_t)) const override {
		const std::filesystem::path full_path = mMountedPath / file_path;
		const std::string full_path_as_string = full_path.string();

		FILE* file{ nullptr };
		auto error = fopen_s(&file, full_path_as_string.c_str(), "rb");
		if (error == 0 && file != nullptr) {
			auto close_file = OnScopeExit{ [file]() { fclose(file); } };

			fseek(file, 0, SEEK_END);
			const std::size_t file_size = ftell(file);
			fseek(file, 0, SEEK_SET);

			if (allocator == nullptr) {
				allocator = malloc;
			}

			const std::size_t allocation_size = file_size + sizeof(FileInfo);
			if (void* buf = allocator(file_size)) {
				void* data = static_cast<void*>(reinterpret_cast<char*>(buf) + 24);
				const auto size_read = fread(data, 1, file_size, file);
				if (size_read != file_size) {
					LogInfo("Could not load file {}, this will either crash or cause glitches...", file_path);
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

		return {};
	}

private:
	std::filesystem::path mMountedPath;
};

struct VirtualFilesystem::VfsMount {
	std::int64_t Priority;
	std::unique_ptr<IVfsMountImpl> MountImpl;
};

VirtualFilesystem::VirtualFilesystem() = default;
VirtualFilesystem::~VirtualFilesystem() = default;

void VirtualFilesystem::MountFolder(std::string_view path, std::int64_t priority) {
	namespace fs = std::filesystem;

	LogInfo("Mounting folder '{}' as a virtual filesystem...", path);

	auto it = std::upper_bound(mMounts.begin(), mMounts.end(), priority, [](std::int64_t prio, const VfsMount& mount) { return mount.Priority < prio; });
	mMounts.insert(it, VfsMount{
			.Priority = priority,
			.MountImpl = std::make_unique<VfsFolderMount>(path)
		});
}

VirtualFilesystem::FileInfo* VirtualFilesystem::LoadFile(std::string_view path, void* (*allocator)(std::size_t))
{
	for (const VfsMount& mount : mMounts) {
		if (FileInfo* loaded_data = mount.MountImpl->LoadFile(path, allocator)) {
			return loaded_data;
		}
	}

	return {};
}
