#pragma once

#include <filesystem>
#include <vector>

class PlaylunkySettings;
class VirtualFilesystem;

void BugFixesMount(VirtualFilesystem& vfs,
                   const std::filesystem::path& db_folder);
bool BugFixesInit(const PlaylunkySettings& settings,
                  const std::filesystem::path& db_folder,
                  const std::filesystem::path& original_data_folder);
void BugFixesCleanup();
