#pragma once

#include <filesystem>
#include <optional>

using ModDatabaseFlagsInt = std::int8_t;
enum ModDatabaseFlags : ModDatabaseFlagsInt {
	ModDatabaseFlags_Files = 1 << 0,
	ModDatabaseFlags_Folders = 1 << 1,
	ModDatabaseFlags_Recurse = 1 << 2
};

class ModDatabase {
public:
	ModDatabase(std::filesystem::path root_folder, ModDatabaseFlags flags);
	~ModDatabase();

	void UpdateDatabase();
	void WriteDatabase();

	template<class FunT>
	void ForEachOutdatedFile(FunT&& fun) {
		for (const ItemDescriptor& file : mFiles) {
			if ((!file.LastKnownWrite.has_value() && file.LastWrite.has_value()) ||
				 (file.LastKnownWrite.has_value() && file.LastWrite.has_value() && file.LastKnownWrite.value() < file.LastWrite.value())) {
				fun(file.Path);
			}
		}
	}

	template<class FunT>
	void ForEachOutdatedFolder(FunT&& fun) {
		for (const ItemDescriptor& folder : mFolders) {
			if ((!folder.LastKnownWrite.has_value() && folder.LastWrite.has_value()) ||
				(folder.LastKnownWrite.has_value() && folder.LastWrite.has_value() && folder.LastKnownWrite.value() < folder.LastWrite.value())) {
				fun(folder.Path);
			}
		}
	}

private:
	const std::filesystem::path mRootFolder;
	const ModDatabaseFlags mFlags;

	struct ItemDescriptor {
		std::filesystem::path Path{};
		std::optional<std::time_t> LastKnownWrite{ std::nullopt };
		std::optional<std::time_t> LastWrite{ std::nullopt };
	};

	std::vector<ItemDescriptor> mFiles;
	std::vector<ItemDescriptor> mFolders;
};
