#pragma once

#include <string>
#include <vector>

void PrintError(std::string message, float time = 20.0f);
void PrintInfo(std::string message, float time = 20.0f);

enum class Alphabet
{
    Latin,
    Cyrillic,
    Japanese,
    ChineseTraditional,
    ChineseSimplified,
    Korean,
    Emoji,

    Last,
};
void ImGuiSetFontFile(std::string font_file, Alphabet alphabet = Alphabet::Latin);
void ImGuiSetFontScale(float font_scale);
struct ImFont* ImGuiGetBestFont(float wanted_size, Alphabet alphabet = Alphabet::Latin);

void DrawImguiOverlay();
void DrawVersionOverlay();

void SetSwapchain(void* swap_chain);
