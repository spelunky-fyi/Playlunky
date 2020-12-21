#pragma once

#include <vector>

std::vector<struct DetourEntry> GetFileIODetours();

void SetVfs(class VirtualFilesystem* vfs);
