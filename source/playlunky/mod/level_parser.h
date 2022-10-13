#pragma once

#include <filesystem>

struct LevelData;
class VirtualFilesystem;

class LevelParser
{
  public:
    LevelData LoadLevel(const VirtualFilesystem& vfs, const std::filesystem::path& backup_folder, const std::filesystem::path& level_file);
    LevelData LoadLevel(const std::filesystem::path& full_level_file);
};
