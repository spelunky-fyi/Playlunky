#pragma once

#include "script_manager.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class SpriteHotLoader;
class SpritePainter;
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

  private:
    std::vector<class ModInfo> mMods;
    std::unique_ptr<SpriteHotLoader> mSpriteHotLoader;
    std::unique_ptr<SpritePainter> mSpritePainter;
    std::unique_ptr<SpriteSheetMerger> mSpriteSheetMerger;
    ScriptManager mScriptManager;
    VirtualFilesystem& mVfs;

    std::filesystem::path mModsRoot;

    bool mForceShowOptions{ false };
    bool mShowCursor{ false };

    bool mDeveloperMode;
    bool mConsoleMode;
    std::string mModSaveGameOverride;
    std::uint64_t mConsoleKey;
    std::uint64_t mConsoleAltKey;
    std::uint64_t mConsoleCloseKey;
};
