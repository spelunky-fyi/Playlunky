#include "file_io.h"

#include "detour_entry.h"
#include "detour_helper.h"
#include "sigscan.h"
#include "log.h"
#include "mod/virtual_filesystem.h"
#include "util/call_once.h"

#include <d3d11.h>
#include <imgui.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <Windows.h>

#include <spel2.h>

struct DetourSwapChainPresent {
	inline static SigScan::Function<HRESULT(__stdcall*) (IDXGISwapChain*, UINT, UINT)> Trampoline{
		.Signature = "\x48\x89\x5c\x24\x10\x48\x89\x74\x24\x20\x55\x57\x41\x56\x48\x8d\x6c\x24\x90\x48\x81\xec\x70\x01\x00\x00"_sig,
		.Module = "dxgi.dll"
	};
	static HRESULT Detour(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
	{
		CallOnce([pSwapChain]() {
			RegisterImguiInitFunc(&ImguiInit);
			InitSwapChainHooks(pSwapChain);
		});
		return Trampoline(pSwapChain, SyncInterval, Flags);
	}

	static void ImguiInit(ImGuiContext* imgui_context) {
		ImGui::SetCurrentContext(imgui_context);

		const bool loaded_font = []() {
			ImGuiIO& io = ImGui::GetIO();
			io.FontAllowUserScaling = true;

			bool loaded_font{ false };

			PWSTR fontdir;
			if (SHGetKnownFolderPath(FOLDERID_Fonts, 0, NULL, &fontdir) == S_OK)
			{
				char fontdir_conv[256]{};
				int length = WideCharToMultiByte(CP_UTF8, 0, fontdir, -1, 0, 0, NULL, NULL);
				assert(length < sizeof(fontdir_conv));
				WideCharToMultiByte(CP_UTF8, 0, fontdir, -1, fontdir_conv, length, NULL, NULL);

				char fontpath[256]{};
				fmt::format_to(fontpath, "{}\\segoeuib.ttf", fontdir_conv);

				if (GetFileAttributesA(fontpath) != INVALID_FILE_ATTRIBUTES)
				{
					loaded_font = io.Fonts->AddFontFromFileTTF(fontpath, 18.0f) || loaded_font;
					loaded_font = io.Fonts->AddFontFromFileTTF(fontpath, 36.0f) || loaded_font;
					loaded_font = io.Fonts->AddFontFromFileTTF(fontpath, 72.0f) || loaded_font;
				}
			}

			CoTaskMemFree(fontdir);
			return loaded_font;
		}();

		if (!loaded_font)
		{
			ImGuiIO& io = ImGui::GetIO();
			io.Fonts->AddFontDefault();
		}
	}

	static void ImguiDraw() {
		ImGui::SetNextWindowSize({ -1, 30 });
		ImGui::SetNextWindowPos({ 0, ImGui::GetIO().DisplaySize.y - 30 });
		ImGui::Begin(
			"Version Overlay",
			nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, .2f), "Playlunky " PLAYLUNKY_VERSION);
		ImGui::End();

		if (!s_Messages.empty()) {
			ImGui::SetNextWindowSize({ -1, -1 });
			ImGui::SetNextWindowPos({ 0, 0 });
			ImGui::Begin(
				"Message Overlay",
				nullptr,
				ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus |
				ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
			for (auto& [message, color, timer] : s_Messages) {
				timer -= 1.0f / 60.0f; // Stupid hax
				const float alpha = std::min(1.0f, timer);
				const ImVec4 faded_color{ color.x, color.y, color.z, color.w * alpha };
				ImGui::TextColored(faded_color, message.c_str());
			}
			std::erase_if(s_Messages, [](const ErrorMessage& msg) { return msg.Timer < 0.0f; });
			ImGui::End();
		}
	}

	static void PrintError(std::string message, float time) {
		s_Messages.push_back(ErrorMessage{ .Message{ std::move(message) }, .Color{ 1.0f, 0.1f, 0.2f, 1.0f }, .Timer{ time } });
	}

	static void PrintInfo(std::string message, float time) {
		s_Messages.push_back(ErrorMessage{ .Message{ std::move(message) }, .Color{ 1.0f, 1.0f, 1.0f, 1.0f }, .Timer{ time } });
	}

	struct ErrorMessage {
		std::string Message;
		ImVec4 Color;
		float Timer;
	};
	inline static std::vector<ErrorMessage> s_Messages;
};

std::vector<DetourEntry> GetImguiDetours() {
	return {
		DetourHelper<DetourSwapChainPresent>::GetDetourEntry("IDXGISwapChain::Present")
	};
}

void PrintError(std::string message, float time) {
	DetourSwapChainPresent::PrintError(std::move(message), time);
}

void PrintInfo(std::string message, float time) {
	DetourSwapChainPresent::PrintInfo(std::move(message), time);
}

void DrawImguiOverlay() {
	DetourSwapChainPresent::ImguiDraw();
}
