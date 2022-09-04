#include "imgui.h"

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
#include <misc/freetype/imgui_freetype.h>
// clang-format on

#include <spel2.h>

#include <deque>

#include <zip_adaptor.h>

using namespace std::string_view_literals;

struct ErrorMessage
{
    std::string Message;
    ImVec4 Color;
    float Timer;
};
inline static std::deque<ErrorMessage> g_Messages;

struct FontConfig
{
    Alphabet alphabet;
    std::array<std::string_view, 2> default_font_files{};
    bool colored_glyphs{ false };
    bool load_big_fonts{ false };
    bool fallback_is_bundled{ false };
    std::array<ImWchar, 3> additional_glyphs{};
};
inline constexpr size_t g_NumFonts{ static_cast<size_t>(Alphabet::Last) };
inline constexpr std::array g_FontConfigs{
    FontConfig{ Alphabet::Latin, { "segoeuib.ttf"sv }, true, true },
    FontConfig{
        Alphabet::Cyrillic,
        { "segoeuib.ttf"sv },
        false,
        false,
        false,
        { 0x2026u, 0x2026u, 0x0u }, // one of the asian fonts has a stupid huge ellipsis, we get it explicitly from this font
    },
    FontConfig{ Alphabet::Japanese, { "YuGothB.ttc"sv, "Meiryo.ttc"sv } },
    FontConfig{ Alphabet::ChineseTraditional, { "simsun.ttc"sv } },
    FontConfig{ Alphabet::ChineseSimplified, { "msjh.ttc"sv } },
    FontConfig{ Alphabet::Korean, { "malgunbd.ttf"sv, "Gulim.ttc"sv } },
    FontConfig{ Alphabet::Emoji, { "seguiemj.ttf"sv }, true, true },
};
static_assert(g_FontConfigs.size() == g_NumFonts);

struct Font
{
    float size;
    ImFont* font{ nullptr };
    std::vector<Alphabet> supported_alphabets;
};
inline static std::array g_Fonts{ Font{ 18.0f }, Font{ 36.0f }, Font{ 72.0f } };
inline static std::array<std::string, g_NumFonts> g_FontFiles{};
inline static float g_FontScale{ 1.0f };

void ImGuiLoadFont()
{
    ImGuiIO& io = ImGui::GetIO();
    io.FontAllowUserScaling = true;

    ImFontConfig imgui_font_config;
    imgui_font_config.EllipsisChar = u'\u2026';
    static constexpr ImWchar emoji_range[] = { 0x1u, 0x1FFFFu, 0x0u };
    std::array<const ImWchar*, g_NumFonts> language_glyph_ranges{
        nullptr,
        io.Fonts->GetGlyphRangesCyrillic(),
        io.Fonts->GetGlyphRangesJapanese(),
        io.Fonts->GetGlyphRangesChineseSimplifiedCommon(),
        io.Fonts->GetGlyphRangesChineseFull(),
        io.Fonts->GetGlyphRangesKorean(),
        emoji_range,
    };

    PWSTR fontdir;
    if (SHGetKnownFolderPath(FOLDERID_Fonts, 0, NULL, &fontdir) == S_OK)
    {
        OnScopeExit free_fontdir{ std::bind_front(CoTaskMemFree, fontdir) };

        for (auto& [size, font, supported_alphabets] : g_Fonts)
        {
            auto load_font_file = [&](std::string_view font_file, const ImWchar* glyph_ranges, const FontConfig& font_config)
            {
                if (font_file.empty())
                {
                    return false;
                }

                auto load_font_impl = [&](const char* font_path)
                {
                    if (font_config.colored_glyphs)
                    {
                        imgui_font_config.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;
                    }
                    else
                    {
                        imgui_font_config.FontBuilderFlags &= ~ImGuiFreeTypeBuilderFlags_LoadColor;
                    }
                    font = io.Fonts->AddFontFromFileTTF(font_path, size * g_FontScale, &imgui_font_config, glyph_ranges);
                    if (font_config.additional_glyphs != decltype(font_config.additional_glyphs){})
                    {
                        font = io.Fonts->AddFontFromFileTTF(font_path, size * g_FontScale, &imgui_font_config, font_config.additional_glyphs.data());
                    }
                };

                namespace fs = std::filesystem;
                fs::path fontpath{ std::filesystem::path{ fontdir } / font_file };
                if (fs::exists(fontpath))
                {
                    load_font_impl(fontpath.string().c_str());
                }
                else if (SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &fontdir) == S_OK)
                {
                    fs::path localfontpath{ std::filesystem::path{ fontdir } / "Microsoft/Windows/Fonts" / font_file };
                    if (fs::exists(localfontpath))
                    {
                        load_font_impl(localfontpath.string().c_str());
                    }
                }

                return font != nullptr;
            };

            const bool big_font = size > g_Fonts.front().size;

            for (auto [font_file, config, glyph_ranges] : zip::zip(g_FontFiles, g_FontConfigs, language_glyph_ranges))
            {
                imgui_font_config.MergeMode = font != nullptr;

                if (!big_font || config.load_big_fonts)
                {
                    const bool loaded_chosen_font = !font_file.empty() && load_font_file(font_file, glyph_ranges, config);
                    const bool loaded_default_font_a = !loaded_chosen_font && load_font_file(config.default_font_files[0], glyph_ranges, config);
                    const bool loaded_default_font_b = !loaded_default_font_a && load_font_file(config.default_font_files[1], glyph_ranges, config);
                    const bool loaded_font_file = loaded_chosen_font || loaded_default_font_a || loaded_default_font_b;
                    if (!loaded_font_file && config.fallback_is_bundled)
                    {
                        font = io.Fonts->AddFontFromMemoryCompressedTTF(PLFont_compressed_data, PLFont_compressed_size, size * g_FontScale, &imgui_font_config, glyph_ranges);
                    }

                    const bool loaded_font = loaded_font_file || config.fallback_is_bundled;
                    supported_alphabets.push_back(config.alphabet);
                }
            }
        }
    }
    else
    {
        for (auto& [size, font, supported_alphabets] : g_Fonts)
        {
            font = io.Fonts->AddFontFromMemoryCompressedTTF(PLFont_compressed_data, PLFont_compressed_size, size * g_FontScale);
            supported_alphabets.push_back(Alphabet::Latin);
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

void ImGuiSetFontFile(std::string font_file, Alphabet alphabet)
{
    g_FontFiles[static_cast<size_t>(alphabet)] = std::move(font_file);
}
void ImGuiSetFontScale(float font_scale)
{
    g_FontScale = font_scale;
}
ImFont* ImGuiGetBestFont(float wanted_size, Alphabet alphabet)
{
    ImFont* best_font{ nullptr };
    for (const auto& [size, font, supported_alphabets] : g_Fonts)
    {
        if (algo::contains(supported_alphabets, alphabet))
        {
            best_font = font;
            if (size > wanted_size)
            {
                break;
            }
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
