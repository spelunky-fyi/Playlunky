#pragma once

#include <vector>

std::vector<struct DetourEntry> GetSaveGameDetours(const class PlaylunkySettings& settings);

void SetSaveGameVfs(class VirtualFilesystem* vfs);
