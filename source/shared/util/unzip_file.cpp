#include "unzip_file.h"

#include "util/on_scope_exit.h"

#include <string>

#include <zip.h>

ZipError UnzipFile(const std::filesystem::path& source_file, const std::filesystem::path& destination_folder)
{
    namespace fs = std::filesystem;

    if (!fs::exists(destination_folder))
    {
        fs::create_directory(destination_folder);
    }
    else if (!fs::is_directory(destination_folder))
    {
        return "Destination already exists and is not a folder...";
    }

    const std::string source_file_string = source_file.string();

    std::int32_t error;
    if (zip* archive = zip_open(source_file_string.c_str(), 0, &error))
    {
        OnScopeExit close_archive([archive]()
                                  { zip_close(archive); });
        for (zip_int64_t i = 0; i < zip_get_num_entries(archive, 0); i++)
        {
            struct zip_stat entry_stat
            {
            };
            if (zip_stat_index(archive, i, 0, &entry_stat) == 0)
            {
                if (entry_stat.size > 0 && std::strstr(entry_stat.name, ".db") == nullptr)
                {
                    if (zip_file_t* zipped_file = zip_fopen_index(archive, i, 0))
                    {
                        const fs::path file_path_in_zip{ entry_stat.name };
                        const fs::path file_path_on_disk = destination_folder / file_path_in_zip.filename();
                        const std::string file_path_on_disk_string = file_path_on_disk.string();

                        FILE* disk_file{ nullptr };
                        if (fopen_s(&disk_file, file_path_on_disk_string.c_str(), "wb") == 0 && disk_file != nullptr)
                        {
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
    else
    {
        char error_buf[256];
        zip_error_to_str(error_buf, sizeof(error_buf), error, errno);
        return error_buf;
    }

    return std::nullopt;
}
