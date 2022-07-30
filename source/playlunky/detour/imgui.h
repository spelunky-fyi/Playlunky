#pragma once

#include <vector>

void PrintError(std::string message, float time = 20.0f);
void PrintInfo(std::string message, float time = 20.0f);

void DrawImguiOverlay();
void DrawVersionOverlay();

void SetSwapchain(void* swap_chain);
