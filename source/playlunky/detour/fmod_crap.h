#pragma once

#include <vector>

std::vector<struct DetourEntry> GetFmodDetours(const class PlaylunkySettings& settings);

void SetFmodVfs(class VirtualFilesystem* vfs);
