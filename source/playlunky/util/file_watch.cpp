#include "file_watch.h"

#pragma warning(push)
#pragma warning(disable : 4927)
#include <FileWatch.hpp>
#pragma warning(pop)

FileWatchId g_CurrentFileWatchId{};
using FileWatcher = filewatch::FileWatch<std::filesystem::path>;
std::unordered_map<FileWatchId, FileWatcher> g_FileWatchers;

FileWatchId AddFileWatch(const std::filesystem::path& file_path, std::function<void(const std::filesystem::path&, filewatch::Event)> cb)
{
    FileWatchId id = g_CurrentFileWatchId++;
    g_FileWatchers.try_emplace(id,
                               file_path,
                               std::move(cb));
    return id;
}
FileWatchId AddFileWatch(const std::filesystem::path& file_path, std::function<void(const std::filesystem::path&, FileEvent)> cb)
{
    return AddFileWatch(
        file_path,
        [cb = std::move(cb)](const std::filesystem::path& file_path, const filewatch::Event change_type)
        {
            cb(file_path, static_cast<FileEvent>(static_cast<int>(change_type)));
        });
}
FileWatchId AddFileGenericWatch(const std::filesystem::path& file_path, std::function<void()> cb)
{
    return AddFileWatch(
        file_path,
        [cb = std::move(cb)](const std::filesystem::path&, const filewatch::Event)
        {
            cb();
        });
}
FileWatchId AddFileAddedWatch(const std::filesystem::path& file_path, std::function<void()> cb)
{
    return AddFileWatch(
        file_path,
        [cb = std::move(cb)](const std::filesystem::path&, const filewatch::Event change_type)
        {
            if (change_type == filewatch::Event::added)
            {
                cb();
            }
        });
}
FileWatchId AddFileModifiedWatch(const std::filesystem::path& file_path, std::function<void()> cb)
{
    return AddFileWatch(
        file_path,
        [cb = std::move(cb)](const std::filesystem::path&, const filewatch::Event change_type)
        {
            if (change_type == filewatch::Event::modified)
            {
                cb();
            }
        });
}
FileWatchId AddFileRemovedWatch(const std::filesystem::path& file_path, std::function<void()> cb)
{
    return AddFileWatch(
        file_path,
        [cb = std::move(cb)](const std::filesystem::path&, const filewatch::Event change_type)
        {
            if (change_type == filewatch::Event::removed)
            {
                cb();
            }
        });
}

void ClearFileWatch(FileWatchId id)
{
    g_FileWatchers.erase(id);
}
