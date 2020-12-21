#pragma once

#include <array>
#include <string>
#include <string_view>
#include <vector>

class ModManager {
public:
	ModManager(std::string_view mods_root, class VirtualFilesystem& vfs);
	~ModManager() = default;

	ModManager() = delete;
	ModManager(const ModManager&) = delete;
	ModManager(ModManager&&) = delete;
	ModManager& operator=(const ModManager&) = delete;
	ModManager& operator=(ModManager&&) = delete;

private:
	static inline constexpr std::array<std::string_view, 4> mReservedFolders{
		".compressed",
		"Extracted",
		"Mods",
		"Overrides"
	};
	static inline constexpr std::array<std::string_view, 1> mRecurseFolders{
		"Packs"
	};

	std::vector<std::string> mMods;
};