#include "mod_database.h"

#include "util/algorithms.h"

#include <fstream>

ModDatabase::ModDatabase(std::filesystem::path root_folder, ModDatabaseFlags flags)
	: mRootFolder(std::move(root_folder))
	, mFlags(flags) {
	namespace fs = std::filesystem;

	if (fs::exists(mRootFolder) && fs::is_directory(mRootFolder)) {
		const fs::path db_path = mRootFolder / ".db" / "mod.db";
		if (fs::exists(db_path) && fs::is_regular_file(db_path)) {
			std::ifstream db_file(db_path, std::ios::binary);

			std::size_t num_files;
			db_file >> num_files;

			mFiles.resize(num_files);
			for (ItemDescriptor& file : mFiles) {
				db_file >> file.Path;

				file.LastKnownWrite.emplace();
				db_file >> file.LastKnownWrite.value();
			}

			if (!db_file.eof()) {
				std::size_t num_folder;
				db_file >> num_folder;

				mFiles.resize(num_folder);
				for (ItemDescriptor& folder : mFiles) {
					db_file >> folder.Path;

					folder.LastKnownWrite.emplace();
					db_file >> folder.LastKnownWrite.value();
				}
			}
		}
	}
}
ModDatabase::~ModDatabase() = default;

void ModDatabase::UpdateDatabase() {
	namespace fs = std::filesystem;

	if (fs::exists(mRootFolder) && fs::is_directory(mRootFolder)) {
		const fs::path db_folder = mRootFolder / ".db";

		auto do_iteration = [this, &db_folder](const fs::path& path) {
			if (!algo::is_sub_path(path, db_folder)) {
				if (fs::is_regular_file(path) && (mFlags & ModDatabaseFlags_Files)) {
					const auto rel_file_path = fs::relative(path, mRootFolder);

					using fs_clock = fs::file_time_type::clock;
					using std::chrono::system_clock;
					const auto system_clock_time = time_point_cast<system_clock::duration>(fs::last_write_time(path) - fs_clock::now() + system_clock::now());
					const auto last_write_time = system_clock::to_time_t(system_clock_time);

					if (ItemDescriptor* existing_file = algo::find_if(mFiles, [&rel_file_path](const auto& file) { return file.Path == rel_file_path; })) {
						existing_file->LastWrite = last_write_time;
					}
					else {
						mFiles.push_back(ItemDescriptor{
								.Path = rel_file_path,
								.LastWrite = last_write_time
							});
					}
				}
				else if (fs::is_directory(path) && (mFlags & ModDatabaseFlags_Folders)) {
					const auto rel_folder_path = fs::relative(path, mRootFolder);

					auto get_last_folder_write_time = [](const fs::path& folder_path) {
						auto oldest_write_time = fs::last_write_time(folder_path);
						for (auto& file_path : fs::recursive_directory_iterator(folder_path)) {
							const auto file_write_time = fs::last_write_time(file_path);
							oldest_write_time = std::min(oldest_write_time, file_write_time);
						}
						return oldest_write_time;
					};

					using fs_clock = fs::file_time_type::clock;
					using std::chrono::system_clock;
					const auto system_clock_time = time_point_cast<system_clock::duration>(get_last_folder_write_time(path) - fs_clock::now() + system_clock::now());
					const auto last_write_time = system_clock::to_time_t(system_clock_time);

					if (ItemDescriptor* existing_file = algo::find_if(mFolders, [&rel_folder_path](const auto& folder) { return folder.Path == rel_folder_path; })) {
						existing_file->LastWrite = last_write_time;
					}
					else {
						mFolders.push_back(ItemDescriptor{
								.Path = rel_folder_path,
								.LastWrite = last_write_time
							});
					}
				}
			}
		};

		if (mFlags & ModDatabaseFlags_Recurse) {
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

		if (mFlags & ModDatabaseFlags_Files) {
			const std::size_t num_files = algo::count_if(mFiles, [](const auto& file) { return file.LastWrite != std::nullopt; });
			db_file << num_files;

			for (ItemDescriptor& file : mFiles) {
				if (file.LastWrite != std::nullopt) {
					db_file << file.Path;
					db_file << file.LastWrite.value();
				}
			}
		}
		else {
			const std::size_t num_files = 0;
			db_file << num_files;
		}

		if (mFlags & ModDatabaseFlags_Folders) {
			const std::size_t num_folders = algo::count_if(mFolders, [](const auto& folder) { return folder.LastWrite != std::nullopt; });
			db_file << num_folders;

			for (ItemDescriptor& folder : mFolders) {
				if (folder.LastWrite != std::nullopt) {
					db_file << folder.Path;
					db_file << folder.LastWrite.value();
				}
			}
		}
		else {
			const std::size_t num_folders = 0;
			db_file << num_folders;
		}
	}
}
