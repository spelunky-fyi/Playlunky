#include "win_main.h"

#include "detour_entry.h"
#include "detour_helper.h"
#include "sigfun.h"
#include "sigscan.h"

#include "../playlunky.h"
#include "log.h"

#include <Windows.h>
#include <string_view>

inline constexpr std::string_view s_SupportedSpelunkyVersion = "1.20.3c";

struct DetourWinMain {
	inline static SigScan::Function<int(__stdcall*)(HINSTANCE, HINSTANCE, LPSTR, int)> Trampoline{
		.Signature = "\x40\x53\x48\x83\xec\x20\x49\x8b\xd8\xff\x15\x2a\x2a\x2a\x2a\x48\x8b\xc8\xba\x02\x00\x00\x00"_sig
	};
	static int Detour(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
		if (void* version_address = SigScan::FindPattern("Spel2.exe", "1.\x2a\x2a.\x2a\x2a\x00"_sig, false)) {
			const char* version_string = static_cast<const char*>(version_address);
			if (version_string != s_SupportedSpelunkyVersion) {
				const auto message = fmt::format("This version of Spelunky 2 is not supported by Playlunky, "
					"please update your game or check if there is a new version of Playlunky available.\n"
					" Spelunky Version: {}\n"
					"Supported Version: {}\n"
					"Playlunky might still function correctly, so just press OK and give it a try."
					"A new version will be released soon that removes this warning.", version_string, s_SupportedSpelunkyVersion);
				MessageBox(NULL, message.c_str(), "Version Mistmacht Detected", MB_OK);
			}
		}
		else {
			const auto message = fmt::format("This version of Spelunky 2 is not supported by Playlunky, "
				"please update your game or check if there is a new version of Playlunky available.\n"
				" Spelunky Version: could not be verified\n"
				"Supported Version: {}\n"
				"Playlunky might still function correctly, so just press OK and give it a try."
				"A new version will be released soon that removes this warning.", s_SupportedSpelunkyVersion);
			MessageBox(NULL, message.c_str(), "Version Mistmacht Detected", MB_OK);
		}

		return Trampoline(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
	}
};

// This is the first function called in WinMain() after the global log is initialized
struct DetourGetSteamHandle {
	inline static SigScan::Function<void* (__stdcall*)()> Trampoline{
		.Signature = "\x40\x53\x48\x83\xec\x20\x48\x8b\x05\x2a\x2a\x2a\x2a\x48\x85\xc0\x75\x76\x48\x39\x05\x2a\x2a\x2a\x2a"_sig
	};
	static void* Detour() {
		static bool s_PlaylunkyInit{ false };
		if (!s_PlaylunkyInit) {
			Playlunky::Get().Init();
			s_PlaylunkyInit = true;
		}
		return Trampoline();
	}
};

std::vector<struct DetourEntry> GetMainDetours() {
	return {
		DetourHelper<DetourWinMain>::GetDetourEntry("WinMain"),
		DetourHelper<DetourGetSteamHandle>::GetDetourEntry("GetSteamHandle")
	};
}
