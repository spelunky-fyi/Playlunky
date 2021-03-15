#include "mod_database.h"

#include "util/algorithms.h"

#include <fstream>

// Previously used magic numbers:
//		0xF00DBAAD -- v0.3.0
//		0xAFFED00F -- v0.4.0
//		0xD00FAFFE -- v0.4.1
//		0xABAD1DEA -- v0.4.2
//		0xBADAB000 -- v0.5.6 (latest)
static constexpr std::uint32_t s_ModDatabaseMagicNumber{ 0xBADAB000 };

ModDatabase::ModDatabase(std::filesystem::path root_folder, ModDatabaseFlags flags)
	: mRootFolder(std::move(root_folder))
	, mFlags(flags) {
	namespace fs = std::filesystem;

	if (fs::exists(mRootFolder) && fs::is_directory(mRootFolder)) {
		const fs::path db_folder = mRootFolder / ".db";
		const fs::path db_path = db_folder / "mod.db";
		if (fs::exists(db_path) && fs::is_regular_file(db_path)) {
			std::ifstream db_file(db_path, std::ios::binary);

			{
				std::uint32_t magic_number;
				db_file.read(reinterpret_cast<char*>(&magic_number), sizeof(magic_number));
				if (magic_number != s_ModDatabaseMagicNumber) {
					db_file.close();
					fs::remove_all(db_folder);
					return;
				}
			}

			db_file.read(reinterpret_cast<char*>(&mWasEnabled), sizeof(mWasEnabled));

			{
				std::size_t num_files;
				db_file.read(reinterpret_cast<char*>(&num_files), sizeof(num_files));

				mFiles.resize(num_files);
				for (ItemDescriptor& file : mFiles) {
					std::size_t path_size;
					db_file.read(reinterpret_cast<char*>(&path_size), sizeof(path_size));

					std::string path(path_size, '\0');
					db_file.read(path.data(), path_size);

					std::time_t last_know_write;
					db_file.read(reinterpret_cast<char*>(&last_know_write), sizeof(last_know_write));

					file.Path = path;
					file.LastKnownWrite = last_know_write;
				}
			}

			{
				std::size_t num_folders;
				db_file.read(reinterpret_cast<char*>(&num_folders), sizeof(num_folders));

				mFolders.resize(num_folders);
				for (ItemDescriptor& folder : mFolders) {
					std::size_t path_size;
					db_file.read(reinterpret_cast<char*>(&path_size), sizeof(path_size));

					std::string path(path_size, '\0');
					db_file.read(path.data(), path_size);

					std::time_t last_know_write;
					db_file.read(reinterpret_cast<char*>(&last_know_write), sizeof(last_know_write));

					folder.Path = path;
					folder.LastKnownWrite = last_know_write;
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

					if (ItemDescriptor* existing_file = algo::find(mFiles, &ItemDescriptor::Path, rel_file_path)) {
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

					if (ItemDescriptor* existing_file = algo::find_if(mFolders, &ItemDescriptor::Path, rel_folder_path)) {
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
			for (auto& path : fs::recursive_directory_iterator(mRootFolder)) {
				do_iteration(path);
			}
		}
		else {
			for (auto& path : fs::directory_iterator(mRootFolder)) {
				do_iteration(path);
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
		db_file.write(reinterpret_cast<const char*>(&s_ModDatabaseMagicNumber), sizeof(s_ModDatabaseMagicNumber));
		db_file.write(reinterpret_cast<const char*>(&mIsEnabled), sizeof(mIsEnabled));

		if (mFlags & ModDatabaseFlags_Files) {
			const std::size_t num_files = algo::count_if(mFiles, [](const auto& file) { return file.LastWrite != std::nullopt; });
			db_file.write(reinterpret_cast<const char*>(&num_files), sizeof(num_files));

			for (ItemDescriptor& file : mFiles) {
				if (file.LastWrite != std::nullopt) {
					const std::string path_string = file.Path.string();
					const std::size_t path_size = path_string.size();
					db_file.write(reinterpret_cast<const char*>(&path_size), sizeof(path_size));
					db_file.write(path_string.data(), path_string.size());

					const std::time_t last_write_time = file.LastWrite.value();
					db_file.write(reinterpret_cast<const char*>(&last_write_time), sizeof(last_write_time));
				}
			}
		}
		else {
			const std::size_t num_files = 0;
			db_file.write(reinterpret_cast<const char*>(&num_files), sizeof(num_files));
		}

		if (mFlags & ModDatabaseFlags_Folders) {
			const std::size_t num_folders = algo::count_if(mFolders, [](const auto& folder) { return folder.LastWrite != std::nullopt; });
			db_file.write(reinterpret_cast<const char*>(&num_folders), sizeof(num_folders));

			for (ItemDescriptor& folder : mFolders) {
				if (folder.LastWrite != std::nullopt) {
					const std::string path_string = folder.Path.string();
					const std::size_t path_size = path_string.size();
					db_file.write(reinterpret_cast<const char*>(&path_size), sizeof(path_size));
					db_file.write(path_string.data(), path_string.size());

					const std::time_t last_write_time = folder.LastWrite.value();
					db_file.write(reinterpret_cast<const char*>(&last_write_time), sizeof(last_write_time));
				}
			}
		}
		else {
			const std::size_t num_folders = 0;
			db_file.write(reinterpret_cast<const char*>(&num_folders), sizeof(num_folders));
		}
	}
}
