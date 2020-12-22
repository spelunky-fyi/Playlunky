#pragma once

#include <filesystem>
#include <optional>

class ModDatabase {
public:
	ModDatabase(std::filesystem::path mod_folder);
	~ModDatabase();

	void UpdateDatabase();
	void WriteDatabase();

	template<class FunT>
	void ForEachOutdatedFile(FunT&& fun) {
		for (const AssetDescriptor& asset : mAssets) {
			if ((!asset.LastKnownWrite.has_value() && asset.LastWrite.has_value()) ||
				 (asset.LastKnownWrite.has_value() && asset.LastWrite.has_value() && asset.LastKnownWrite.value() < asset.LastWrite.value())) {
				fun(asset.Path);
			}
		}
	}

private:
	std::filesystem::path mModFolder;

	struct AssetDescriptor {
		std::filesystem::path Path{};
		std::optional<std::time_t> LastKnownWrite{ std::nullopt };
		std::optional<std::time_t> LastWrite{ std::nullopt };
	};
	std::vector<AssetDescriptor> mAssets;
};
