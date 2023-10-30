#include "script_manager.h"

#include "log.h"
#include "playlunky.h"
#include "playlunky_settings.h"
#include "util/algorithms.h"
#include "util/function_pointer.h"

#include <spel2.h>

#include <imgui.h>

inline constexpr bool g_DisableScriptMods = false;

ScriptManager::~ScriptManager() = default;

bool ScriptManager::RegisterModWithScript(std::string_view mod_name, const std::filesystem::path& main_path, std::int64_t priority, bool enabled)
{
    if constexpr (g_DisableScriptMods)
    {
        return true;
    }
    else
    {
        if (algo::contains(mMods, &RegisteredMainScript::ModName, mod_name))
        {
            return false;
        }
        auto it = std::upper_bound(mMods.begin(), mMods.end(), priority, [](std::int64_t prio, const RegisteredMainScript& mod)
                                   { return mod.Priority > prio; });
        mMods.insert(it, RegisteredMainScript{ .ModName{ std::string{ mod_name } }, .MainPath{ main_path }, .Priority{ priority }, .Enabled{ enabled }, .ScriptEnabled{ enabled } });
        if (enabled)
        {
            Playlunky::Get().RegisterModType(ModType::Script);
        }
        return true;
    }
}

void ScriptManager::CommitScripts(const class PlaylunkySettings& settings)
{
    if constexpr (g_DisableScriptMods)
    {
        return;
    }
    else
    {
        {
            const bool speedrun_mode = settings.GetBool("general_settings", "speedrun_mode", false);
            const bool enable_console = settings.GetBool("script_settings", "enable_developer_console", false);
            if (!speedrun_mode && enable_console)
            {
                mConsole = CreateConsole();
                SpelunkyConsole_LoadHistory(mConsole, "console_history.txt");
                SpelunkyConsole_SetMaxHistorySize(mConsole, settings.GetInt("script_settings", "console_history_size", 20));
            }
        }

        for (RegisteredMainScript& mod : mMods)
        {
            if (mod.Enabled)
            {
                const std::string path_string = mod.MainPath.string();
                mod.Script = SpelunkyScriptPointer{ Spelunky_CreateScript(path_string.c_str(), false) };
                if (mod.Script != nullptr)
                {
                    mod.TestScriptResult();

                    SpelunkyScript_GetMeta(
                        mod.Script.get(),
                        FunctionPointer<SpelunkyScript_MetaFun, struct ScriptManagerGetScriptMetaAtInit>(
                            [&](const SpelunkyScriptMeta& meta)
                            {
                                mod.Unsafe = meta.unsafe;
                                mod.OnlineSafe = meta.online_safe;
                                if (meta.unsafe)
                                {
                                    mod.ScriptEnabled = false;
                                }
                                else
                                {
                                    SpelunkyScript_SetEnabled(mod.Script.get(), mod.ScriptEnabled);
                                }
                            }));
                }
            }
        }
    }
}
void ScriptManager::RefreshScripts()
{
    for (RegisteredMainScript& mod : mMods)
    {
        const std::string path_string = mod.MainPath.string();
        mod.Script.reset();
        mod.Script = SpelunkyScriptPointer{
            Spelunky_CreateScript(path_string.c_str(), mod.ScriptEnabled),
        };
        mod.LastResult.clear();
    }
}
void ScriptManager::Update()
{
    if (mConsole)
    {
        SpelunkyConsole_ConsumeMessages(
            mConsole,
            FunctionPointer<SpelunkyConsole_MessageFun, struct ScriptManagerConsumeMessages>(
                [this](const char* message)
                {
                    if (message != nullptr)
                    {
                        LogInfoScreen("[Dev-Console]: {}", message);
                    }
                }));
    }
    for (RegisteredMainScript& mod : mMods)
    {
        if (mod.Script != nullptr)
        {
            mod.TestScriptResult();

            std::size_t message_time = mod.MessageTime;
            SpelunkyScript_LoopMessages(
                mod.Script.get(),
                FunctionPointer<SpelunkyScript_MessageFun, struct ScriptManagerLoopMessages>(
                    [&](SpelunkyScriptMessage message)
                    {
                        if (message.Message != nullptr && message.TimeMilliSecond > mod.MessageTime)
                        {
                            message_time = std::max(message_time, message.TimeMilliSecond);
                            LogInfoScreen("[{}]: {}", mod.ModName, message.Message);
                        }
                    }));
            mod.MessageTime = message_time;
        }
    }
}

bool ScriptManager::NeedsWindowDraw()
{
    if (mMods.empty() && mConsole == nullptr && !g_DisableScriptMods)
    {
        return false;
    }

    return true;
}
void ScriptManager::WindowDraw()
{
    if (mMods.empty() && mConsole == nullptr && !g_DisableScriptMods)
    {
        return;
    }

    if constexpr (g_DisableScriptMods)
    {
        ImGui::Separator();
        ImGui::TextWrapped("%s", "Script mods are currently unavailable... Wait for version 0.11.0 to get script support back... Thank you for your patience  <3");
    }

    if (mConsole)
    {
        ImGui::Separator();
        ImGui::TextUnformatted("Dev-Console");
        SpelunkyConsole_DrawOptions(mConsole);
    }

    for (RegisteredMainScript& mod : mMods)
    {
        if ((mod.Script != nullptr) && mod.Enabled)
        {

            SpelunkyScript_GetMeta(
                mod.Script.get(),
                FunctionPointer<SpelunkyScript_MetaFun, struct ScriptManagerGetScriptMetaAtDraw>(
                    [&](const SpelunkyScriptMeta& meta)
                    {
                        char by_author[128]{};
                        fmt::format_to(by_author, "by {}", meta.author);
                        const auto author_cursor_pos =
                            ImGui::GetCursorPosX() + ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(by_author).x - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x;

                        ImGui::Separator();
                        if (ImGui::Checkbox(meta.name, &mod.ScriptEnabled))
                        {
                            SpelunkyScript_SetEnabled(mod.Script.get(), mod.ScriptEnabled);
                        }

                        if (meta.version != nullptr && std::strlen(meta.version) > 0)
                        {
                            ImGui::SameLine();
                            ImGui::TextUnformatted("|");
                            ImGui::SameLine();
                            ImGui::Text("Version %s", meta.version);
                        }

                        ImGui::SameLine();
                        ImGui::SetCursorPosX(author_cursor_pos);
                        ImGui::TextUnformatted(by_author);

                        if (mod.Unsafe && !mod.ScriptEnabled)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                            ImGui::TextWrapped("Warning: This mod uses unsafe commands, it could delete your files and download viruses. It probably doesn't, but it could. Only enable this mod if you trust the author.");
                            ImGui::PopStyleColor();
                        }

                        if (mod.OnlineSafe && mod.ScriptEnabled)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.4f, 0.0f, 1.0f));
                            ImGui::TextWrapped("Note: This mod is tagged as online-safe, but caution is still advised. Likely all players require the mod and other caveats may apply. For example a restart of the whole game is advised before switching back to local play.");
                            ImGui::PopStyleColor();
                        }

                        if (meta.description != nullptr && std::strlen(meta.description) > 0)
                        {
                            ImGui::TextWrapped("%s", meta.description);
                        }

                        if (mod.ScriptEnabled)
                        {
                            SpelunkyScript_DrawOptions(mod.Script.get());
                        }
                    }));
        }
    }
}
void ScriptManager::Draw()
{
    if (mMods.empty() && mConsole == nullptr && !g_DisableScriptMods)
    {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    if (SpelunkyState_GetScreen() == SpelunkyScreen::Online &&
        algo::contains(mMods, &RegisteredMainScript::EnabledNotOnlineSafe, true))
    {
        ImGui::SetNextWindowSize({ -1, -1 });
        ImGui::Begin(
            "Online Warning Overlay",
            nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
        {
            const int num_colors = 64;
            const float frequency = 10.0f / num_colors;
            for (int i = 0; i < num_colors; ++i)
            {
                const float red = std::sin(frequency * i + 0) * 0.5f + 0.5f;
                const float green = std::sin(frequency * i + 2) * 0.5f + 0.5f;
                const float blue = std::sin(frequency * i + 4) * 0.5f + 0.5f;

                ImGui::TextColored(ImVec4(red, green, blue, 1.0f), "Do not use script mods online! Your game will not work! Press Ctrl+F4 and disable your mods! ");
                for (int j = 0; j < 4; j++)
                {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(red, green, blue, 1.0f), "Do not use script mods online! Your game will not work! Press Ctrl+F4 and disable your mods! ");
                }
            }
        }
        ImGui::SetWindowPos({ ImGui::GetIO().DisplaySize.x / 2 - ImGui::GetWindowWidth() / 2, ImGui::GetIO().DisplaySize.y / 2 - ImGui::GetWindowHeight() / 2 }, ImGuiCond_Always);
        ImGui::End();
    }

    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::Begin(
        "Clickhandler",
        nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    if (mConsole)
    {
        SpelunkyConsole_Draw(mConsole, draw_list);

        if (SpelunkyConsole_HasNewHistory(mConsole))
        {
            SpelunkyConsole_SaveHistory(mConsole, "console_history.txt");
        }
    }
    for (RegisteredMainScript& mod : mMods)
    {
        if (mod.Script != nullptr)
        {
            SpelunkyScript_Draw(mod.Script.get(), draw_list);
        }
    }

    ImGui::End();
}

bool ScriptManager::IsConsoleToggled()
{
    return mConsole
               ? SpelunkyConsole_IsToggled(mConsole)
               : false;
}
void ScriptManager::ToggleConsole()
{
    if (mConsole)
    {
        SpelunkyConsole_Toggle(mConsole);
    }
}

void ScriptManager::RegisteredMainScript::TestScriptResult()
{
    if (Script != nullptr)
    {
        using namespace std::literals::string_view_literals;
        char res[512];
        SpelunkyScript_GetResult(Script.get(), res, sizeof(res));
        if (res != "Got metadata"sv && res != "OK"sv && res != LastResult)
        {
            LogError("Lua Error:\n\tMod: {}\n\tError: {}", ModName, res);
            LastResult = res;
        }
    }
}

void ScriptManager::SpelunkyScriptDeleter::operator()(SpelunkyScript* script) const
{
    Spelunky_FreeScript(script);
}
