#include "mod_database.h"

#include "util/algorithms.h"
#include "util/on_scope_exit.h"

#include <Windows.h>
#include <fstream>

// Previously used magic numbers:
//		0xF00DBAAD -- v0.3.0
//		0xAFFED00F -- v0.4.0
//		0xD00FAFFE -- v0.4.1
//		0xABAD1DEA -- v0.4.2
//		0xBADAB000 -- v0.5.6
//		0xABCDEF16 -- v0.7.0
//		0xABACAB00 -- v0.7.1
//		0xBEDD1BED -- v0.7.2
//		0xDEE25CA2 -- v0.7.3
//		0x0BAFF1ED -- v0.8.0
//		0x0BEECAB0 -- v0.8.1
//		0x0CABBA6E -- v0.10.0
//		0xFEEDFACE -- v0.11.0
static constexpr std::uint32_t s_ModDatabaseMagicNumber{ 0xFEEDFACE };

ModDatabase::ModDatabase(std::filesystem::path database_folder, std::filesystem::path mod_folder, ModDatabaseFlags flags)
    : mDatabaseFolder(std::move(database_folder)), mModFolder(std::move(mod_folder)), mFlags(flags)
{
    namespace fs = std::filesystem;

    const bool is_global_db = algo::is_sub_path(mDatabaseFolder, mModFolder);
    if (!is_global_db)
    {
        const fs::path old_db_folder = mModFolder / ".db";
        if (fs::exists(old_db_folder) && fs::is_directory(old_db_folder))
        {
            fs::remove_all(old_db_folder);
        }
    }

    if (fs::exists(mDatabaseFolder) && fs::is_directory(mDatabaseFolder))
    {
        const fs::path db_path = mDatabaseFolder / "mod.db";
        if (fs::exists(db_path) && fs::is_regular_file(db_path))
        {
            std::ifstream db_file(db_path, std::ios::binary);

            {
                std::uint32_t magic_number;
                db_file.read(reinterpret_cast<char*>(&magic_number), sizeof(magic_number));
                if (magic_number != s_ModDatabaseMagicNumber)
                {
                    db_file.close();
                    fs::remove_all(mDatabaseFolder);
                    return;
                }
            }

            db_file.read(reinterpret_cast<char*>(&mWasEnabled), sizeof(mWasEnabled));
            mIsEnabled = mWasEnabled;

            {
                std::size_t num_files;
                db_file.read(reinterpret_cast<char*>(&num_files), sizeof(num_files));

                mFiles.resize(num_files);
                for (ItemDescriptor& file : mFiles)
                {
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
                for (ItemDescriptor& folder : mFolders)
                {
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

            {
                std::size_t num_settings;
                db_file.read(reinterpret_cast<char*>(&num_settings), sizeof(num_settings));

                mSettings.resize(num_settings);
                for (AdditionalSetting& setting : mSettings)
                {
                    std::size_t name_size;
                    db_file.read(reinterpret_cast<char*>(&name_size), sizeof(name_size));

                    std::string name(name_size, '\0');
                    db_file.read(name.data(), name_size);

                    bool value;
                    db_file.read(reinterpret_cast<char*>(&value), sizeof(value));

                    setting.Name = std::move(name);
                    setting.Value = value;
                }
            }

            {
                std::size_t mod_info_size;
                db_file.read(reinterpret_cast<char*>(&mod_info_size), sizeof(mod_info_size));

                mModInfo.resize(mod_info_size);
                db_file.read(mModInfo.data(), mod_info_size);
            }
        }
    }
}
ModDatabase::~ModDatabase() = default;

void ModDatabase::UpdateDatabase()
{
    namespace fs = std::filesystem;
    if (fs::exists(mModFolder) && fs::is_directory(mModFolder))
    {
        static auto get_last_write_time = [](const fs::path& file_path)
        {
            HANDLE file = CreateFile(file_path.string().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
            if (file != INVALID_HANDLE_VALUE)
            {
                OnScopeExit close_handle{ [file]()
                                          { CloseHandle(file); } };

                FILETIME last_write_time;
                if (GetFileTime(file, NULL, NULL, &last_write_time))
                {
                    SYSTEMTIME system_time;
                    if (FileTimeToSystemTime(&last_write_time, &system_time))
                    {
                        std::tm tm;
                        tm.tm_sec = system_time.wSecond;
                        tm.tm_min = system_time.wMinute;
                        tm.tm_hour = system_time.wHour;
                        tm.tm_mday = system_time.wDay;
                        tm.tm_mon = system_time.wMonth - 1;
                        tm.tm_year = system_time.wYear - 1900;
                        tm.tm_isdst = -1;
                        return std::mktime(&tm);
                    }
                }
            }
            return time_t{ 0 };
        };

        auto do_iteration = [this](const fs::path& path)
        {
            if (algo::is_sub_path(path, mDatabaseFolder))
            {
                return false;
            }
            else
            {
                if (fs::is_regular_file(path) && (mFlags & ModDatabaseFlags_Files))
                {
                    const auto rel_file_path = fs::relative(path, mModFolder);

                    using fs_clock = fs::file_time_type::clock;
                    using std::chrono::system_clock;
                    const auto last_write_time = get_last_write_time(path);

                    if (ItemDescriptor* existing_file = algo::find(mFiles, &ItemDescriptor::Path, rel_file_path))
                    {
                        existing_file->LastWrite = last_write_time;
                    }
                    else
                    {
                        mFiles.push_back(ItemDescriptor{
                            .Path = rel_file_path,
                            .LastWrite = last_write_time });
                    }
                }
                else if (fs::is_directory(path) && (mFlags & ModDatabaseFlags_Folders))
                {
                    const auto rel_folder_path = fs::relative(path, mModFolder);

                    auto get_last_folder_write_time = [](const fs::path& folder_path)
                    {
                        auto oldest_write_time = get_last_write_time(folder_path);
                        for (auto& file_path : fs::recursive_directory_iterator(folder_path))
                        {
                            const auto file_write_time = get_last_write_time(file_path);
                            oldest_write_time = std::min(oldest_write_time, file_write_time);
                        }
                        return oldest_write_time;
                    };

                    using fs_clock = fs::file_time_type::clock;
                    using std::chrono::system_clock;
                    const auto last_write_time = get_last_folder_write_time(path);

                    if (ItemDescriptor* existing_file = algo::find(mFolders, &ItemDescriptor::Path, rel_folder_path))
                    {
                        existing_file->LastWrite = last_write_time;
                    }
                    else
                    {
                        mFolders.push_back(ItemDescriptor{
                            .Path = rel_folder_path,
                            .LastWrite = last_write_time });
                    }
                }
                return true;
            }
        };

        if (mFlags & ModDatabaseFlags_Recurse)
        {
            auto iter_recurse = [&do_iteration](const auto& path, auto& self) -> void
            {
                for (auto& sub_path : fs::directory_iterator(path))
                {
                    if (do_iteration(sub_path))
                    {
                        if (fs::is_directory(sub_path))
                        {
                            self(sub_path, self);
                        }
                    }
                }
            };
            iter_recurse(mModFolder, iter_recurse);
        }
        else
        {
            for (auto& path : fs::directory_iterator(mModFolder))
            {
                (void)do_iteration(path);
            }
        }
    }
}
void ModDatabase::WriteDatabase() const
{
    namespace fs = std::filesystem;

    if (!fs::exists(mModFolder))
    {
        if (fs::exists(mDatabaseFolder))
        {
            fs::remove_all(mDatabaseFolder);
        }
        return;
    }
    else if (!fs::exists(mDatabaseFolder) || !fs::is_directory(mDatabaseFolder))
    {
        if (fs::exists(mDatabaseFolder))
        {
            fs::remove_all(mDatabaseFolder);
        }
        fs::create_directories(mDatabaseFolder);
    }

    if (fs::exists(mDatabaseFolder) && fs::is_directory(mDatabaseFolder))
    {
        const fs::path db_path = mDatabaseFolder / "mod.db";
        if (fs::exists(db_path))
        {
            fs::remove_all(db_path);
        }

        std::ofstream db_file(db_path, std::ios::binary);
        db_file.write(reinterpret_cast<const char*>(&s_ModDatabaseMagicNumber), sizeof(s_ModDatabaseMagicNumber));
        db_file.write(reinterpret_cast<const char*>(&mIsEnabled), sizeof(mIsEnabled));

        if (mFlags & ModDatabaseFlags_Files)
        {
            const std::size_t num_files = algo::count_if(mFiles, [](const auto& file)
                                                         { return file.Exists(); });
            db_file.write(reinterpret_cast<const char*>(&num_files), sizeof(num_files));

            for (const ItemDescriptor& file : mFiles)
            {
                if (file.Exists())
                {
                    const std::string path_string = file.Path.string();
                    const std::size_t path_size = path_string.size();
                    db_file.write(reinterpret_cast<const char*>(&path_size), sizeof(path_size));
                    db_file.write(path_string.data(), path_string.size());

                    const std::time_t last_write_time = file.LastWrite.value();
                    db_file.write(reinterpret_cast<const char*>(&last_write_time), sizeof(last_write_time));
                }
            }
        }
        else
        {
            const std::size_t num_files = 0;
            db_file.write(reinterpret_cast<const char*>(&num_files), sizeof(num_files));
        }

        if (mFlags & ModDatabaseFlags_Folders)
        {
            const std::size_t num_folders = algo::count_if(mFolders, [](const auto& folder)
                                                           { return folder.Exists(); });
            db_file.write(reinterpret_cast<const char*>(&num_folders), sizeof(num_folders));

            for (const ItemDescriptor& folder : mFolders)
            {
                if (folder.Exists())
                {
                    const std::string path_string = folder.Path.string();
                    const std::size_t path_size = path_string.size();
                    db_file.write(reinterpret_cast<const char*>(&path_size), sizeof(path_size));
                    db_file.write(path_string.data(), path_string.size());

                    const std::time_t last_write_time = folder.LastWrite.value();
                    db_file.write(reinterpret_cast<const char*>(&last_write_time), sizeof(last_write_time));
                }
            }
        }
        else
        {
            const std::size_t num_folders = 0;
            db_file.write(reinterpret_cast<const char*>(&num_folders), sizeof(num_folders));
        }

        const std::size_t num_settings = mSettings.size();
        db_file.write(reinterpret_cast<const char*>(&num_settings), sizeof(num_settings));
        for (const AdditionalSetting& setting : mSettings)
        {
            const std::size_t name_size = setting.Name.size();
            db_file.write(reinterpret_cast<const char*>(&name_size), sizeof(name_size));
            db_file.write(setting.Name.data(), setting.Name.size());

            db_file.write(reinterpret_cast<const char*>(&setting.Value), sizeof(setting.Value));
        }

        {
            std::size_t mod_info_size = mModInfo.size();
            db_file.write(reinterpret_cast<char*>(&mod_info_size), sizeof(mod_info_size));
            db_file.write(mModInfo.data(), mod_info_size);
        }
    }
}

bool ModDatabase::GetAdditionalSetting(std::string_view name, bool default_value) const
{
    if (const AdditionalSetting* setting = algo::find(mSettings, &AdditionalSetting::Name, name))
    {
        return setting->Value;
    }
    return default_value;
}
void ModDatabase::SetAdditionalSetting(std::string_view name, bool value)
{
    if (AdditionalSetting* setting = algo::find(mSettings, &AdditionalSetting::Name, name))
    {
        setting->Value = value;
        return;
    }
    mSettings.push_back(AdditionalSetting{ .Name{ std::string{ name } }, .Value{ value } });
}
