#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

class SpelunkyScript;

class ScriptManager {
public:
	bool RegisterModWithScript(std::string_view mod_name, const std::filesystem::path& main_path, bool enabled);

	void CommitScripts();
	void RefreshScripts();
	void Update();
	void Draw();

	void ToggleForceShowOptions() { mForceShowOptions = !mForceShowOptions; }

private:
	struct RegisteredMainScript{
		std::string ModName;
		std::filesystem::path MainPath;
		bool Enabled;
		bool ScriptEnabled;
		std::string LastResult;
		SpelunkyScript* Script{ nullptr };

		void TestScriptResult();
	};
	std::vector<RegisteredMainScript> mMods;
	bool mForceShowOptions{ false };
};