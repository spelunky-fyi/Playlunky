#pragma once

#include <filesystem>
#include <string>
#include <vector>

class VirtualFilesystem;

bool MergeShaders(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder,
	const std::filesystem::path& shader_file, VirtualFilesystem& vfs);
