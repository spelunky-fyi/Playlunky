#include "file_io.h"

#include "detour_entry.h"
#include "detour_helper.h"
#include "sigscan.h"
#include "log.h"
#include "mod/virtual_filesystem.h"
#include "util/call_once.h"

#include <d3d11.h>
#include <Windows.h>

#include <imgui.h>
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
			RegisterImguiDrawFunc(&ImguiDraw);
			InitSwapChainHooks(pSwapChain);
		});
		return Trampoline(pSwapChain, SyncInterval, Flags);
	}

	static void ImguiInit(ImGuiContext* imgui_context) {
		ImGui::SetCurrentContext(imgui_context);
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

		if (!s_ErrorMessages.empty()) {
			ImGui::SetNextWindowSize({ -1, -1 });
			ImGui::SetNextWindowPos({ 0, 0 });
			ImGui::Begin(
				"Error Overlay",
				nullptr,
				ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus |
				ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
			for (auto& [message, timer] : s_ErrorMessages) {
				timer -= 1.0f / 60.0f; // Stupid hax
				ImGui::TextColored(ImVec4(1.0f, 0.1f, 0.2f, 1.0f), message.c_str());
			}
			std::erase_if(s_ErrorMessages, [](const ErrorMessage& msg) { return msg.Timer < 0.0f; });
			ImGui::End();
		}
	}
	
	static void PrintError(std::string message, float time) {
		s_ErrorMessages.push_back(ErrorMessage{ .Message{ std::move(message) }, .Timer{ time } });
	}

	struct ErrorMessage {
		std::string Message;
		float Timer;
	};
	inline static std::vector<ErrorMessage> s_ErrorMessages;
};

std::vector<DetourEntry> GetImguiDetours() {
	return {
		DetourHelper<DetourSwapChainPresent>::GetDetourEntry("IDXGISwapChain::Present")
	};
}

void PrintError(std::string message, float time)
{
	DetourSwapChainPresent::PrintError(std::move(message), time);
}
