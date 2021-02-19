#pragma once

#include <vector>

std::vector<struct DetourEntry> GetFmodDetours();

void SetFmodVfs(class VirtualFilesystem* vfs);
