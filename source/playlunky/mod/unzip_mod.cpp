#include "unzip_mod.h"

#include "log.h"

#include <fmt/format.h>
#include <zip.h>

void UnzipMod(const std::filesystem::path& zip_file) {
	namespace fs = std::filesystem;

	const auto mod_folder = fs::path{ zip_file }.replace_extension("");
	if (!fs::exists(mod_folder)) {
		fs::create_directory(mod_folder);
	}

	const auto zip_string = zip_file.string();

	std::int32_t error;
	if (zip* archive = zip_open(zip_string.c_str(), 0, &error)) {
		for (zip_int64_t i = 0; i < zip_get_num_entries(archive, 0); i++) {
			struct zip_stat entry_stat {};
			if (zip_stat_index(archive, i, 0, &entry_stat) == 0) {
				if (entry_stat.size > 0 && std::strstr(entry_stat.name, ".db") == nullptr) {
					if (zip_file_t* zipped_file = zip_fopen_index(archive, i, 0)) {
						const fs::path file_path_in_zip{ entry_stat.name };
						const fs::path file_path_on_disk = mod_folder / file_path_in_zip.filename();
						const std::string file_path_on_disk_string = file_path_on_disk.string();

						FILE* disk_file{ nullptr };
						if (fopen_s(&disk_file, file_path_on_disk_string.c_str(), "wb") == 0 && disk_file != nullptr) {
							const std::size_t zipped_file_size = entry_stat.size;

							std::vector<std::byte> zipped_file_contents(zipped_file_size);
							const std::size_t read_size = zip_fread(zipped_file, zipped_file_contents.data(), zipped_file_size);
							// TODO: Error

							fwrite(zipped_file_contents.data(), 1, zipped_file_size, disk_file);
							fflush(disk_file);

							fclose(disk_file);
						}

						zip_fclose(zipped_file);
					}
				}
			}
		}
	}
	else {
		char error_buf[256];
		zip_error_to_str(error_buf, sizeof(error_buf), error, errno);
		LogInfo("Can't open zip archive '{}': {}/n", zip_string, error_buf);
	}
}