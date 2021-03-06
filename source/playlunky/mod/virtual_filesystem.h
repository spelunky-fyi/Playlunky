#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

class VirtualFilesystem {
public:
	VirtualFilesystem();
	~VirtualFilesystem();

	VirtualFilesystem(const VirtualFilesystem&) = delete;
	VirtualFilesystem(VirtualFilesystem&&) = delete;
	VirtualFilesystem& operator=(const VirtualFilesystem&) = delete;
	VirtualFilesystem& operator=(VirtualFilesystem&&) = delete;

	void MountFolder(std::string_view path, std::int64_t priority);
	void BindPathes(std::vector<std::string_view> pathes);

	struct FileInfo {
		void* Data{ nullptr };
		int _member_1{ 0 };
		int DataSize{ 0 };
		int AllocationSize{ 0 };
		int _member_4{ 0 };
	};
	FileInfo* LoadFile(const char* path, void* (*allocator)(std::size_t) = nullptr) const;
	FileInfo* LoadSpecificFile(const char* specific_path, void* (*allocator)(std::size_t) = nullptr) const;
	std::optional<std::filesystem::path> GetFilePath(const std::filesystem::path& path) const;
	std::vector<std::filesystem::path> GetAllFilePaths(const std::filesystem::path& path) const;

private:
	using BoundPathes = std::vector<std::string_view>;
	BoundPathes* GetBoundPathes(std::string_view path);
	BoundPathes* GetBoundPathes(const BoundPathes& pathes);
	const BoundPathes* GetBoundPathes(std::string_view path) const;
	const BoundPathes* GetBoundPathes(const BoundPathes& pathes) const;

	struct VfsMount;
	std::vector<VfsMount> mMounts;

	std::vector<BoundPathes> m_BoundPathes;
};
