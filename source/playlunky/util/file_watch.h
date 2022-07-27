#pragma once

#include <filesystem>

enum class FileEvent
{
	Added,
	Removed,
	Modified,
	RenamedOld,
	RenamedNew,
};

void AddFileWatch(const std::filesystem::path& file_path, std::function<void(const std::filesystem::path&, FileEvent)> cb);
void AddFileModifiedWatch(const std::filesystem::path& file_path, std::function<void()> cb);
