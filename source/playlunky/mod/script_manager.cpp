#include "script_manager.h"

#include "log.h"
#include "util/algorithms.h"

#include <spel2.h>

#include <imgui.h>

bool ScriptManager::RegisterModWithScript(std::string_view mod_name, const std::filesystem::path& main_path, bool enabled) {
	if (algo::contains(mMods, &RegisteredMainScript::ModName, mod_name)) {
		return false;
	}
	mMods.push_back(RegisteredMainScript{ .ModName{ std::string{ mod_name } }, .MainPath{ main_path }, .Enabled{ enabled }, .ScriptEnabled{ enabled } });
	return true;
}

void ScriptManager::CommitScripts() {
	for (RegisteredMainScript& mod : mMods) {
		const std::string path_string = mod.MainPath.string();
		mod.Script = CreateScript(path_string.c_str(), mod.ScriptEnabled);
		mod.TestScriptResult();
	}
}
void ScriptManager::RefreshScripts() {
	for (RegisteredMainScript& mod : mMods) {
		const std::string path_string = mod.MainPath.string();
		SpelunkyScipt_ReloadScript(mod.Script, path_string.c_str());
		mod.LastResult.clear();
	}
}
void ScriptManager::Update() {
	for (RegisteredMainScript& mod : mMods) {
		if (mod.Script) {
			SpelunkyScript_Update(mod.Script);
			mod.TestScriptResult();
		}
	}
}
void ScriptManager::Draw() {
	ImGuiIO& io = ImGui::GetIO();

	if (mForceShowOptions || SpelunkyState_GetScreen() == SpelunkyScreen::Menu) {
		ImGui::SetNextWindowSize({ io.DisplaySize.x / 4, io.DisplaySize.y });
		ImGui::SetNextWindowPos({ io.DisplaySize.x * 3 / 4, 0 });
		ImGui::Begin(
			"Mod Options",
			NULL,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration);

		ImGui::TextUnformatted("Mod Options");

		for (RegisteredMainScript& mod : mMods) {
			if (mod.Script && mod.Enabled) {
				ImGui::Separator();
				if (ImGui::Checkbox(mod.ModName.c_str(), &mod.ScriptEnabled)) {
					SpelunkyScipt_SetEnabled(mod.Script, mod.ScriptEnabled);
				}
				SpelunkyScript_DrawOptions(mod.Script);
			}
		}

		ImGui::End();
	}

	ImGui::SetNextWindowSize(io.DisplaySize);
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::Begin(
		"Clickhandler",
		NULL,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
	for (RegisteredMainScript& mod : mMods) {
		if (mod.Script) {
			SpelunkyScript_Draw(mod.Script, draw_list);
		}
	}

	ImGui::End();
}

void ScriptManager::RegisteredMainScript::TestScriptResult() {
	using namespace std::literals::string_view_literals;
	if (const char* res = SpelunkyScript_GetResult(Script)) {
		if (res != "Got metadata"sv && res != "OK"sv && res != LastResult) {
			LogError("Lua Error:\n\tMod: {}\n\tError: {}", ModName, res);
			LastResult = res;
		}
	}
}
