#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

class SpelunkyScript;

class ScriptManager
{
  public:
    bool RegisterModWithScript(std::string_view mod_name, const std::filesystem::path& main_path, std::int64_t priority, bool enabled);

    void CommitScripts();
    void RefreshScripts();
    void Update();
    void Draw();

    void ToggleForceShowOptions()
    {
        mForceShowOptions = !mForceShowOptions;
    }

  private:
    struct RegisteredMainScript
    {
        std::string ModName;
        std::filesystem::path MainPath;
        std::int64_t Priority;
        bool Enabled;
        bool ScriptEnabled;
        bool Unsafe;
        std::size_t MessageTime;
        std::string LastResult;
        SpelunkyScript* Script{ nullptr };

        void TestScriptResult();
    };
    std::vector<RegisteredMainScript> mMods;
    bool mForceShowOptions{ false };
    bool mShowCursor{ false };
};