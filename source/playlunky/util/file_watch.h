#pragma once

#include <cstdint>
#include <filesystem>

enum class FileEvent
{
    Added,
    Removed,
    Modified,
    RenamedOld,
    RenamedNew,
};

using FileWatchId = uint32_t;
FileWatchId AddFileWatch(const std::filesystem::path& file_path, std::function<void(const std::filesystem::path&, FileEvent)> cb);
FileWatchId AddFileGenericWatch(const std::filesystem::path& file_path, std::function<void()> cb);
FileWatchId AddFileAddedWatch(const std::filesystem::path& file_path, std::function<void()> cb);
FileWatchId AddFileModifiedWatch(const std::filesystem::path& file_path, std::function<void()> cb);
FileWatchId AddFileRemovedWatch(const std::filesystem::path& file_path, std::function<void()> cb);

void ClearFileWatch(FileWatchId id);
