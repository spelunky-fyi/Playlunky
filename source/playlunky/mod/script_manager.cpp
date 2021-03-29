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
		if (mod.Script != nullptr) {
			SpelunkyScript_Update(mod.Script);
			mod.TestScriptResult();

			const std::size_t num_messages = SpelunkyScript_GetNumMessages(mod.Script);
			for (std::size_t i = 0; i < num_messages; i++) {
				SpelunkyScriptMessage message = SpelunkyScript_GetMessage(mod.Script, i);
				if (message.Message != nullptr && message.TimeMilliSecond >= mod.MessageTime) {
					LogInfoScreen("[{}]: {}", mod.ModName, message.Message);
				}
			}
			mod.MessageTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		}
	}
}
void ScriptManager::Draw() {
	if (mMods.empty()) {
		return;
	}

	ImGuiIO& io = ImGui::GetIO();

	if (SpelunkyState_GetScreen() == SpelunkyScreen::Online && algo::contains(mMods, &RegisteredMainScript::ScriptEnabled, true))
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

	if (mForceShowOptions || SpelunkyState_GetScreen() == SpelunkyScreen::Menu) {
		if (!GetShowCursor()) {
			SetShowCursor(true);
		}

		ImGui::SetNextWindowSize({ io.DisplaySize.x / 4, io.DisplaySize.y });
		ImGui::SetNextWindowPos({ io.DisplaySize.x * 3 / 4, 0 });
		ImGui::Begin(
			"Mod Options",
			nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration);
		ImGui::PushItemWidth(100.0f);

		ImGui::TextUnformatted("Mod Options");

		for (RegisteredMainScript& mod : mMods) {
			if ((mod.Script != nullptr) && mod.Enabled) {
				SpelunkyScriptMeta meta = SpelunkyScript_GetMeta(mod.Script);

				const std::string by_author = fmt::format("by {}", meta.author);
				const auto author_cursor_pos =
					ImGui::GetCursorPosX() + ImGui::GetWindowWidth() - ImGui::CalcTextSize(by_author.c_str()).x
					- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x;

				ImGui::Separator();
				if (ImGui::Checkbox(meta.name, &mod.ScriptEnabled)) {
					SpelunkyScipt_SetEnabled(mod.Script, mod.ScriptEnabled);
				}

				if (meta.version != nullptr && std::strlen(meta.version) > 0) {
					ImGui::SameLine();
					ImGui::TextUnformatted("|");
					ImGui::SameLine();
					ImGui::Text("Version %s", meta.version);
				}

				ImGui::SameLine();
				ImGui::SetCursorPosX(author_cursor_pos);
				ImGui::TextUnformatted(by_author.c_str());

				if (meta.description != nullptr && std::strlen(meta.description) > 0) {
					ImGui::TextUnformatted(meta.description);
				}

				SpelunkyScript_DrawOptions(mod.Script);
			}
		}

		ImGui::PopItemWidth();
		ImGui::End();
	}
	else if (GetShowCursor()) {
		SetShowCursor(false);
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
	for (RegisteredMainScript& mod : mMods) {
		if (mod.Script != nullptr) {
			SpelunkyScript_Draw(mod.Script, draw_list);
		}
	}

	ImGui::End();
}

void ScriptManager::RegisteredMainScript::TestScriptResult() {
	if (Script != nullptr) {
		using namespace std::literals::string_view_literals;
		if (const char* res = SpelunkyScript_GetResult(Script)) {
			if (res != "Got metadata"sv && res != "OK"sv && res != LastResult) {
				LogError("Lua Error:\n\tMod: {}\n\tError: {}", ModName, res);
				LastResult = res;
			}
		}
	}
}
