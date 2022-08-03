#pragma once

#include <string>
#include <vector>

void PrintError(std::string message, float time = 20.0f);
void PrintInfo(std::string message, float time = 20.0f);

void ImGuiSetFontFile(std::string font_file);
void ImGuiSetFontScale(float font_scale);
struct ImFont* ImGuiGetBestFont(float wanted_size);

void DrawImguiOverlay();
void DrawVersionOverlay();

void SetSwapchain(void* swap_chain);
