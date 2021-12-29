#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

class VirtualFilesystem;

class StringMerger
{
  public:
    StringMerger() = default;
    StringMerger(const StringMerger&) = delete;
    StringMerger(StringMerger&&) = delete;
    StringMerger& operator=(const StringMerger&) = delete;
    StringMerger& operator=(StringMerger&&) = delete;
    ~StringMerger() = default;

    bool RegisterOutdatedStringTable(std::string_view table);
    bool RegisterModdedStringTable(std::string_view table);

    bool NeedsRegen() const
    {
        return mNeedsRegen;
    }

    bool MergeStrings(
        const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder, const std::filesystem::path& hash_file_path, bool speedrun_mode, VirtualFilesystem& vfs);

  private:
    bool mNeedsRegen{ false };

    struct OutdatedStringTable
    {
        std::uint8_t Index;
        bool Modded{ false };
    };
    std::vector<OutdatedStringTable> mOutdatedStringTables;
};
