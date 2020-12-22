#include "mod_database.h"

#include "util/algorithms.h"

#include <fstream>

ModDatabase::ModDatabase(std::filesystem::path mod_folder)
	: mModFolder(std::move(mod_folder)) {
	namespace fs = std::filesystem;

	if (fs::exists(mModFolder) && fs::is_directory(mModFolder)) {
		const fs::path db_path = mModFolder / ".db" / "mod.db";
		if (fs::exists(db_path) && fs::is_regular_file(db_path)) {
			std::ifstream db_file(db_path, std::ios::binary);

			std::size_t num_assets;
			db_file >> num_assets;

			mAssets.resize(num_assets);
			for (AssetDescriptor& asset : mAssets) {
				db_file >> asset.Path;

				asset.LastKnownWrite.emplace();
				db_file >> asset.LastKnownWrite.value();
			}
		}
	}
}
ModDatabase::~ModDatabase() = default;

void ModDatabase::UpdateDatabase() {
	namespace fs = std::filesystem;

	if (fs::exists(mModFolder) && fs::is_directory(mModFolder)) {
		const fs::path db_folder = mModFolder / ".db";
		
		for (auto& asset_path : fs::recursive_directory_iterator(mModFolder)) {
			if (fs::is_regular_file(asset_path) && !algo::is_sub_path(asset_path, db_folder)) {
				const auto rel_asset_path = fs::relative(asset_path, mModFolder);

				using fs_clock = fs::file_time_type::clock;
				using std::chrono::system_clock;
				const auto system_clock_time = time_point_cast<system_clock::duration>(fs::last_write_time(asset_path) - fs_clock::now() + system_clock::now());
				const auto last_write_time = system_clock::to_time_t(system_clock_time);

				if (AssetDescriptor* existing_asset = algo::find_if(mAssets, [&rel_asset_path](const auto& asset) { return asset.Path == rel_asset_path; })) {
					existing_asset->LastWrite = last_write_time;
				}
				else {
					mAssets.push_back(AssetDescriptor{
							.Path = rel_asset_path,
							.LastWrite = last_write_time
						});
				}
			}
		}
	}
}
void ModDatabase::WriteDatabase() {
	namespace fs = std::filesystem;

	if (fs::exists(mModFolder) && fs::is_directory(mModFolder)) {
		const fs::path db_folder = mModFolder / ".db";
		if (!fs::exists(db_folder) || !fs::is_directory(db_folder)) {
			if (fs::exists(db_folder)) {
				fs::remove_all(db_folder);
			}
			fs::create_directory(db_folder);
		}

		const fs::path db_path = db_folder / "mod.db";
		if (fs::exists(db_path)) {
			fs::remove_all(db_path);
		}

		std::ofstream db_file(db_path, std::ios::binary);

		const std::size_t num_assets = algo::count_if(mAssets, [](const auto& asset) { return asset.LastWrite != std::nullopt; });
		db_file << num_assets;

		for (AssetDescriptor& asset : mAssets) {
			if (asset.LastWrite != std::nullopt) {
				db_file << asset.Path;
				db_file << asset.LastWrite.value();
			}
		}
	}
}
