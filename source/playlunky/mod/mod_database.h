#pragma once

#include <filesystem>
#include <optional>

#include "log.h"

using ModDatabaseFlagsInt = std::int8_t;
enum ModDatabaseFlags : ModDatabaseFlagsInt {
	ModDatabaseFlags_Files = 1 << 0,
	ModDatabaseFlags_Folders = 1 << 1,
	ModDatabaseFlags_Recurse = 1 << 2
};

class ModDatabase {
public:
	ModDatabase(std::filesystem::path database_folder, std::filesystem::path mod_folder, ModDatabaseFlags flags);
	~ModDatabase();

	void SetEnabled(bool enabled) { mIsEnabled = enabled; }

	void UpdateDatabase();
	void WriteDatabase() const;

	bool GetAdditionalSetting(std::string_view name, bool default_value) const;
	void SetAdditionalSetting(std::string_view name, bool value);

	template<class FunT>
	requires std::is_invocable_v<FunT, std::filesystem::path, bool, bool, std::optional<bool>>
	void ForEachFile(FunT&& fun) {
		const auto new_enabled_state = mWasEnabled != mIsEnabled
			? std::optional<bool>{ mIsEnabled }
			: std::nullopt;
		for (const ItemDescriptor& file : mFiles) {
			const bool outdated = file.IsNew() || file.IsChanged();
			const bool deleted = file.IsDeleted();
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
			const bool outdated = folder.IsNew() || folder.IsChanged();
			const bool deleted = folder.IsDeleted();
			fun(folder.Path, outdated, deleted, new_enabled_state);
		}
	}

private:
	const std::filesystem::path mDatabaseFolder;
	const std::filesystem::path mModFolder;
	const ModDatabaseFlags mFlags;

	struct ItemDescriptor {
		std::filesystem::path Path{};
		std::optional<std::time_t> LastKnownWrite{ std::nullopt };
		std::optional<std::time_t> LastWrite{ std::nullopt };

		bool Exists() const {
			return LastWrite.has_value();
		}
		bool Existed() const {
			return LastKnownWrite.has_value();
		}
		bool IsNew() const {
			return !Existed() && Exists();
		}
		bool IsChanged() const {
			return Exists() && Existed() && LastKnownWrite.value() < LastWrite.value();
		}
		bool IsDeleted() const {
			return Existed() && !Exists();
		}
	};
	std::vector<ItemDescriptor> mFiles;
	std::vector<ItemDescriptor> mFolders;
	
	struct AdditionalSetting {
		std::string Name;
		bool Value;
	};
	std::vector<AdditionalSetting> mSettings;

	bool mWasEnabled{ false };
	bool mIsEnabled{ true };
};
