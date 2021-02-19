#pragma once

#include <vector>

std::vector<struct DetourEntry> GetFileIODetours();

void SetFileIOVfs(class VirtualFilesystem* vfs);
