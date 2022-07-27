#include "file_watch.h"

#pragma warning(push)
#pragma warning(disable : 4927)
#include <FileWatch.hpp>
#pragma warning(pop)

std::vector<std::unique_ptr<filewatch::FileWatch<std::filesystem::path>>> g_FileWatchers;

void AddFileWatch(const std::filesystem::path& file_path, std::function<void(const std::filesystem::path&, FileEvent)> cb)
{
	g_FileWatchers.push_back(std::make_unique<filewatch::FileWatch<std::filesystem::path>>(
		file_path,
		[cb = std::move(cb)](const std::filesystem::path& file_path, const filewatch::Event change_type)
		{
			cb(file_path, static_cast<FileEvent>(static_cast<int>(change_type)));
		}));
}
void AddFileModifiedWatch(const std::filesystem::path& file_path, std::function<void()> cb)
{
	g_FileWatchers.push_back(std::make_unique<filewatch::FileWatch<std::filesystem::path>>(
		file_path,
		[cb = std::move(cb)](const std::filesystem::path&, const filewatch::Event change_type)
		{
			if (change_type == filewatch::Event::modified)
			{
				cb();
			}
		}));
}
