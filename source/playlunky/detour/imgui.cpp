#include "detour_entry.h"
#include "detour_helper.h"
#include "log.h"
#include "mod/virtual_filesystem.h"
#include "playlunky.h"
#include "playlunky_settings.h"
#include "sigscan.h"
#include "util/call_once.h"
#include "version.h"

// clang-format off
#include <Windows.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <d3d11.h>
#include <imgui.h>
// clang-format on

#include <spel2.h>

#include <deque>

struct ErrorMessage
{
    std::string Message;
    ImVec4 Color;
    float Timer;
};
inline static std::deque<ErrorMessage> g_Messages;

void ImguiInit(ImGuiContext* imgui_context)
{
    ImGui::SetCurrentContext(imgui_context);

    {
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = "imgui_playlunky.ini";
    }

    const bool loaded_font = []()
    {
        ImGuiIO& io = ImGui::GetIO();
        io.FontAllowUserScaling = true;

        bool loaded_font{ false };

        PWSTR fontdir;
        if (SHGetKnownFolderPath(FOLDERID_Fonts, 0, NULL, &fontdir) == S_OK)
        {
            char fontdir_conv[256]{};
            int length = WideCharToMultiByte(CP_UTF8, 0, fontdir, -1, 0, 0, NULL, NULL);
            assert(length < sizeof(fontdir_conv));
            WideCharToMultiByte(CP_UTF8, 0, fontdir, -1, fontdir_conv, length, NULL, NULL);

            char fontpath[256]{};
            fmt::format_to(fontpath, "{}\\segoeuib.ttf", fontdir_conv);

            if (GetFileAttributesA(fontpath) != INVALID_FILE_ATTRIBUTES)
            {
                loaded_font = io.Fonts->AddFontFromFileTTF(fontpath, 18.0f) || loaded_font;
                loaded_font = io.Fonts->AddFontFromFileTTF(fontpath, 36.0f) || loaded_font;
                loaded_font = io.Fonts->AddFontFromFileTTF(fontpath, 72.0f) || loaded_font;
            }
        }

        CoTaskMemFree(fontdir);
        return loaded_font;
    }();

    if (!loaded_font)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontDefault();
    }
}

void ImguiDraw()
{
    if (!g_Messages.empty())
    {
        ImGui::SetNextWindowSize({ -1, -1 });
        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::Begin(
            "Message Overlay",
            nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
        for (auto& [message, color, timer] : g_Messages)
        {
            timer -= 1.0f / 60.0f;
            const float alpha = std::min(1.0f, timer);
            const ImVec4 faded_color{ color.x, color.y, color.z, color.w * alpha };
            ImGui::TextColored(faded_color, message.c_str());
        }
        std::erase_if(g_Messages, [](const ErrorMessage& msg)
                      { return msg.Timer < 0.0f; });
        ImGui::End();
    }
}

void PrintError(std::string message, float time)
{
    g_Messages.push_back(ErrorMessage{ .Message{ std::move(message) }, .Color{ 1.0f, 0.1f, 0.2f, 1.0f }, .Timer{ time } });
    if (g_Messages.size() > 25)
    {
        g_Messages.pop_front();
    }
}

void PrintInfo(std::string message, float time)
{
    g_Messages.push_back(ErrorMessage{ .Message{ std::move(message) }, .Color{ 1.0f, 1.0f, 1.0f, 1.0f }, .Timer{ time } });
    if (g_Messages.size() > 25)
    {
        g_Messages.pop_front();
    }
}

void DrawImguiOverlay()
{
    ImguiDraw();
}
void DrawVersionOverlay()
{
    const float overlay_alpha = []() -> float
    {
        // if (static_cast<int>(SpelunkyState_GetScreen()) <= static_cast<int>(SpelunkyScreen::Menu))
        //{
        //     return 0.25f;
        // }
        return 0.004f;
    }();

    const std::string_view version = playlunky_version();
    const std::string full_version_str = fmt::format("Playlunky {}", version);
    const float color[4]{ 0.7f, 0.7f, 0.7f, overlay_alpha };
    const float scale{ 0.0005f };
    const auto [w, h] = Spelunky_DrawTextSize(full_version_str.c_str(), scale, scale, 0);
    Spelunky_DrawText(full_version_str.c_str(), -1.0f, -1.0f + std::abs(h) / 2.0f, scale, scale, color, 0, 0);
}

void SetSwapchain(void* swap_chain)
{
    Spelunky_RegisterImguiInitFunc(&ImguiInit);
    Spelunky_InitSwapChainHooks(static_cast<IDXGISwapChain*>(swap_chain));
}
