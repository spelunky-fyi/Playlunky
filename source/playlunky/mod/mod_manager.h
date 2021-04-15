#pragma once

#include "script_manager.h"

#include <string>
#include <string_view>
#include <vector>

class ModManager {
public:
	ModManager(std::string_view mods_root, const class PlaylunkySettings& settings, class VirtualFilesystem& vfs);
	~ModManager();

	ModManager() = delete;
	ModManager(const ModManager&) = delete;
	ModManager(ModManager&&) = delete;
	ModManager& operator=(const ModManager&) = delete;
	ModManager& operator=(ModManager&&) = delete;

	void PostGameInit();

	bool OnInput(std::uint32_t msg, std::uint64_t w_param, std::int64_t l_param);
	void Update();
	void Draw();

private:
	std::vector<class ModInfo> mMods;
	ScriptManager mScriptManager;
	bool mDeveloperMode;
};
