#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

class SpelunkyScript;
class SpelunkyConsole;

class ScriptManager
{
  public:
    ~ScriptManager();

    bool RegisterModWithScript(std::string_view mod_name, const std::filesystem::path& main_path, std::int64_t priority, bool enabled);

    void CommitScripts(const class PlaylunkySettings& settings);
    void RefreshScripts();
    void Update();

    bool NeedsWindowDraw();
    void WindowDraw();
    void Draw();

    bool IsConsoleToggled();
    void ToggleConsole();

  private:
    struct SpelunkyScriptDeleter
    {
        void operator()(SpelunkyScript* script) const;
    };
    using SpelunkyScriptPointer = std::unique_ptr<SpelunkyScript, SpelunkyScriptDeleter>;

    struct RegisteredMainScript
    {
        std::string ModName;
        std::filesystem::path MainPath;
        std::int64_t Priority;
        bool Enabled;
        bool ScriptEnabled;
        bool Unsafe;
        bool OnlineSafe;
        std::size_t MessageTime;
        std::string LastResult;
        SpelunkyScriptPointer Script;

        void TestScriptResult();
    };
    std::vector<RegisteredMainScript> mMods;

    SpelunkyConsole* mConsole{ nullptr };
};
