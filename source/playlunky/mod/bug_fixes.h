#pragma once

#include <filesystem>
#include <vector>

class PlaylunkySettings;
class VirtualFilesystem;

bool BugFixesInit(VirtualFilesystem& vfs,
                  const PlaylunkySettings& settings,
                  const std::filesystem::path& db_folder,
                  const std::filesystem::path& original_data_folder);
void BugFixesUpdate();
void BugFixesCleanup();
