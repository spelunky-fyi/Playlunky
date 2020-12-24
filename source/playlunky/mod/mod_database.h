#pragma once

#include <filesystem>
#include <optional>

class ModDatabase {
public:
	ModDatabase(std::filesystem::path root_folder, bool recursive);
	~ModDatabase();

	void UpdateDatabase();
	void WriteDatabase();

	template<class FunT>
	void ForEachOutdatedFile(FunT&& fun) {
		for (const FileDescriptor& asset : mFiles) {
			if ((!asset.LastKnownWrite.has_value() && asset.LastWrite.has_value()) ||
				 (asset.LastKnownWrite.has_value() && asset.LastWrite.has_value() && asset.LastKnownWrite.value() < asset.LastWrite.value())) {
				fun(asset.Path);
			}
		}
	}

private:
	const std::filesystem::path mRootFolder;
	const bool mRecurse;

	struct FileDescriptor {
		std::filesystem::path Path{};
		std::optional<std::time_t> LastKnownWrite{ std::nullopt };
		std::optional<std::time_t> LastWrite{ std::nullopt };
	};
	std::vector<FileDescriptor> mFiles;
};
