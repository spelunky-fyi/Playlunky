#pragma once

#include <vector>

std::vector<struct DetourEntry> GetImguiDetours();

void PrintError(std::string message, float time = 20.0f);

void DrawImguiOverlay();
