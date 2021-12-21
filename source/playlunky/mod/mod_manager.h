#pragma once

#include "script_manager.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class SpriteHotLoader;
class SpriteSheetMerger;

class ModManager
{
  public:
    ModManager(std::string_view mods_root, const class PlaylunkySettings& settings, class VirtualFilesystem& vfs);
    ~ModManager();

    ModManager() = delete;
    ModManager(const ModManager&) = delete;
    ModManager(ModManager&&) = delete;
    ModManager& operator=(const ModManager&) = delete;
    ModManager& operator=(ModManager&&) = delete;

    void PostGameInit(const class PlaylunkySettings& settings);

    bool OnInput(std::uint32_t msg, std::uint64_t w_param, std::int64_t l_param);
    void Update();
    void Draw();
    void Destroy();

  private:
    std::vector<class ModInfo> mMods;
    std::unique_ptr<SpriteHotLoader> m_SpriteHotLoader;
    std::unique_ptr<SpriteSheetMerger> m_SpriteSheetMerger;
    ScriptManager mScriptManager;
    VirtualFilesystem* m_Vfs;

    std::filesystem::path m_ModsRoot;

    bool mDeveloperMode;
    bool mConsoleMode;
    std::string m_ModSaveGameOverride;
    std::uint64_t mConsoleKey;
    std::uint64_t mConsoleAltKey;
    std::uint64_t mConsoleCloseKey;
};
