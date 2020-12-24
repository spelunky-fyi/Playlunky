#include "mod_database.h"

#include "util/algorithms.h"

#include <fstream>

ModDatabase::ModDatabase(std::filesystem::path root_folder, bool recursive)
	: mRootFolder(std::move(root_folder))
	, mRecurse(recursive) {
	namespace fs = std::filesystem;

	if (fs::exists(mRootFolder) && fs::is_directory(mRootFolder)) {
		const fs::path db_path = mRootFolder / ".db" / "mod.db";
		if (fs::exists(db_path) && fs::is_regular_file(db_path)) {
			std::ifstream db_file(db_path, std::ios::binary);

			std::size_t num_assets;
			db_file >> num_assets;

			mFiles.resize(num_assets);
			for (FileDescriptor& asset : mFiles) {
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

	if (fs::exists(mRootFolder) && fs::is_directory(mRootFolder)) {
		const fs::path db_folder = mRootFolder / ".db";

		auto do_iteration = [this, &db_folder](const fs::path& file_path) {
			if (fs::is_regular_file(file_path) && !algo::is_sub_path(file_path, db_folder)) {
				const auto rel_file_path = fs::relative(file_path, mRootFolder);

				using fs_clock = fs::file_time_type::clock;
				using std::chrono::system_clock;
				const auto system_clock_time = time_point_cast<system_clock::duration>(fs::last_write_time(file_path) - fs_clock::now() + system_clock::now());
				const auto last_write_time = system_clock::to_time_t(system_clock_time);

				if (FileDescriptor* existing_asset = algo::find_if(mFiles, [&rel_file_path](const auto& asset) { return asset.Path == rel_file_path; })) {
					existing_asset->LastWrite = last_write_time;
				}
				else {
					mFiles.push_back(FileDescriptor{
							.Path = rel_file_path,
							.LastWrite = last_write_time
						});
				}
			}
		};

		if (mRecurse) {
			for (auto& file_path : fs::recursive_directory_iterator(mRootFolder)) {
				do_iteration(file_path);
			}
		}
		else {
			for (auto& file_path : fs::directory_iterator(mRootFolder)) {
				do_iteration(file_path);
			}
		}
	}
}
void ModDatabase::WriteDatabase() {
	namespace fs = std::filesystem;

	if (fs::exists(mRootFolder) && fs::is_directory(mRootFolder)) {
		const fs::path db_folder = mRootFolder / ".db";
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

		const std::size_t num_assets = algo::count_if(mFiles, [](const auto& asset) { return asset.LastWrite != std::nullopt; });
		db_file << num_assets;

		for (FileDescriptor& asset : mFiles) {
			if (asset.LastWrite != std::nullopt) {
				db_file << asset.Path;
				db_file << asset.LastWrite.value();
			}
		}
	}
}
