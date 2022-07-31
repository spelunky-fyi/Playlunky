#include "detour_entry.h"
#include "detour_helper.h"
#include "log.h"
#include "mod/virtual_filesystem.h"
#include "playlunky.h"
#include "playlunky_settings.h"
#include "plfont.h"
#include "sigscan.h"
#include "util/algorithms.h"
#include "util/call_once.h"
#include "util/on_scope_exit.h"
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

struct Font
{
    float size;
    ImFont* font{ nullptr };
};
inline static std::array g_Fonts{ Font{ 14.0f }, Font{ 32.0f }, Font{ 72.0f } };
inline static std::string g_FontFile{ "segoeuib.ttf" };

void ImGuiLoadFont()
{
    ImGuiIO& io = ImGui::GetIO();
    io.FontAllowUserScaling = true;

    if (!g_FontFile.empty())
    {
        PWSTR fontdir;
        if (SHGetKnownFolderPath(FOLDERID_Fonts, 0, NULL, &fontdir) == S_OK)
        {
            OnScopeExit free_fontdir{ std::bind_front(CoTaskMemFree, fontdir) };

            namespace fs = std::filesystem;
            fs::path fontpath{ std::filesystem::path{ fontdir } / g_FontFile };
            if (fs::exists(fontpath))
            {
                for (auto& [size, font] : g_Fonts)
                {
                    font = io.Fonts->AddFontFromFileTTF(fontpath.string().c_str(), size);
                }
            }
            else if (SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &fontdir) == S_OK)
            {
                fs::path localfontpath{ std::filesystem::path{ fontdir } / "Microsoft/Windows/Fonts" / g_FontFile };
                if (fs::exists(localfontpath))
                {
                    for (auto& [size, font] : g_Fonts)
                    {
                        font = io.Fonts->AddFontFromFileTTF(localfontpath.string().c_str(), size);
                    }
                }
            }
        }
    }

    if (algo::contains(g_Fonts, &Font::font, nullptr))
    {
        for (auto& [size, font] : g_Fonts)
        {
            font = io.Fonts->AddFontFromMemoryCompressedTTF(PLFont_compressed_data, PLFont_compressed_size, size);
        }
    }
}

void ImguiInit(ImGuiContext* imgui_context)
{
    ImGui::SetCurrentContext(imgui_context);

    {
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = "imgui_playlunky.ini";
    }

    ImGuiLoadFont();
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

void ImGuiSetFontFile(std::string font_file)
{
    g_FontFile = std::move(font_file);
}
ImFont* ImGuiGetBestFont(float wanted_size)
{
    ImFont* best_font{ nullptr };
    for (const auto& [size, font] : g_Fonts)
    {
        best_font = font;
        if (size > wanted_size)
        {
            break;
        }
    }

    return best_font;
}

void DrawImguiOverlay()
{
    ImguiDraw();
}
void DrawVersionOverlay()
{
    const float overlay_alpha = []() -> float
    {
        if (static_cast<int>(SpelunkyState_GetScreen()) <= static_cast<int>(SpelunkyScreen::Menu))
        {
            return 0.25f;
        }
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
