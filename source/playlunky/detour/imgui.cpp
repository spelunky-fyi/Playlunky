#include "file_io.h"

#include "detour_entry.h"
#include "detour_helper.h"
#include "log.h"
#include "mod/virtual_filesystem.h"
#include "playlunky.h"
#include "playlunky_settings.h"
#include "sigscan.h"
#include "util/call_once.h"

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
    static const bool speedrun_mode = Playlunky::Get().GetSettings().GetBool("general_settings", "speedrun_mode", false);
    if (!speedrun_mode)
    {
        const std::uint8_t overlay_alpha = []() -> std::uint8_t
        {
            if (static_cast<int>(SpelunkyState_GetScreen()) <= static_cast<int>(SpelunkyScreen::Menu))
            {
                return 77;
            }
            return 2;
        }();

        ImGui::SetNextWindowSize({ -1, 30 });
        ImGui::SetNextWindowPos({ 0, ImGui::GetIO().DisplaySize.y - 30 });
        ImGui::Begin(
            "Version Overlay",
            nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
        ImGui::TextColored(ImColor(0xffffff | (overlay_alpha << 24)), "Playlunky " PLAYLUNKY_VERSION);
        ImGui::End();
    }

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

void SetSwapchain(void* swap_chain)
{
    RegisterImguiInitFunc(&ImguiInit);
    InitSwapChainHooks(static_cast<IDXGISwapChain*>(swap_chain));
}
