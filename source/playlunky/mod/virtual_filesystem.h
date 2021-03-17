#pragma once

#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <variant>
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

	// Binding pathes makes sure that only one of the bound files can be loaded
	void BindPathes(std::vector<std::string_view> pathes);

	// Interface for runtime loading
	struct FileInfo {
		void* Data{ nullptr };
		int _member_1{ 0 };
		int DataSize{ 0 };
		int AllocationSize{ 0 };
		int _member_4{ 0 };
	};
	FileInfo* LoadFile(const char* path, void* (*allocator)(std::size_t) = nullptr) const;

	// Interface for loading during preprocessing
	std::optional<std::filesystem::path> GetFilePath(const std::filesystem::path& path) const;
	std::optional<std::filesystem::path> GetRandomFilePath(const std::filesystem::path& path) const;
	std::vector<std::filesystem::path> GetAllFilePaths(const std::filesystem::path& path) const;

private:
	using BoundPathes = std::vector<std::string_view>;
	BoundPathes* GetBoundPathes(std::string_view path);
	BoundPathes* GetBoundPathes(const BoundPathes& pathes);
	const BoundPathes* GetBoundPathes(std::string_view path) const;
	const BoundPathes* GetBoundPathes(const BoundPathes& pathes) const;

	struct VfsMount;
	std::vector<VfsMount> mMounts;

	using CachedRandomFileKey = std::variant<const BoundPathes*, std::filesystem::path>;
	struct CachedRandomFile {
		CachedRandomFileKey TargetPath;
		std::optional<std::filesystem::path> ResultPath;
	};
	mutable std::mutex m_RandomCacheMutex;
	mutable std::vector<CachedRandomFile> m_RandomCache;

	std::vector<BoundPathes> m_BoundPathes;
};
