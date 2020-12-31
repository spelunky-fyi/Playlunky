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

	void SetEnabled(bool enabled) { mIsEnabled = enabled; }
	void UpdateDatabase();
	void WriteDatabase();

	template<class FunT>
	requires std::is_invocable_v<FunT, std::filesystem::path, bool, bool, std::optional<bool>>
	void ForEachFile(FunT&& fun) {
		const auto new_enabled_state = mWasEnabled != mIsEnabled
			? std::optional<bool>{ mIsEnabled }
			: std::nullopt;
		for (const ItemDescriptor& file : mFiles) {
			const bool outdated = (!file.LastKnownWrite.has_value() && file.LastWrite.has_value()) ||
				(file.LastKnownWrite.has_value() && file.LastWrite.has_value() && file.LastKnownWrite.value() < file.LastWrite.value());
			const bool deleted = file.LastKnownWrite.has_value() && !file.LastWrite.has_value();
			fun(file.Path, outdated, deleted, new_enabled_state);
		}
	}

	template<class FunT>
	requires std::is_invocable_v<FunT, std::filesystem::path, bool, bool, std::optional<bool>>
	void ForEachFolder(FunT&& fun) {
		const auto new_enabled_state = mWasEnabled != mIsEnabled
			? std::optional<bool>{ mIsEnabled }
			: std::nullopt;
		for (const ItemDescriptor& folder : mFolders) {
			const bool outdated = (!folder.LastKnownWrite.has_value() && folder.LastWrite.has_value()) ||
				(folder.LastKnownWrite.has_value() && folder.LastWrite.has_value() && folder.LastKnownWrite.value() < folder.LastWrite.value());
			const bool deleted = folder.LastKnownWrite.has_value() && !folder.LastWrite.has_value();
			fun(folder.Path, outdated, deleted, new_enabled_state);
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

	bool mWasEnabled{ false };
	bool mIsEnabled{ true };
};
