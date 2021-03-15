#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

class SpelunkyScript;

class ScriptManager {
public:
	bool RegisterModWithScript(std::string_view mod_name, const std::filesystem::path& main_path);

	void CommitScripts();
	void Update();
	void Draw();

private:
	struct RegisteredMainScript{
		std::string ModName;
		std::filesystem::path MainPath;
		SpelunkyScript* Script{ nullptr };
	};
	std::vector<RegisteredMainScript> mMods;
};