#include "debug.h"

#include "detour_entry.h"
#include "detour_helper.h"
#include "sigfun.h"
#include "log.h"
#include "util.h"

#include <Windows.h>
#include <cstdint>
#include <filesystem>

struct DetourMainForScyllaHide
{
	inline static SigScan::Function<int(__stdcall*)(HINSTANCE, HINSTANCE, LPSTR, int)> Trampoline{
		.Signature = "\x40\x53\x48\x83\xec\x20\x49\x8b\xd8\xff\x2a\x2a\x2a\x2a\x00\x48\x8b\xc8\xba\x02\x00\x00\x00\xff\x15\x2a\x2a\x2a\x2a"
	};
	static int Detour(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
	{
		LogInfo("Trying to inject ScyllaHide...");
		if (std::filesystem::exists("ScyllaHide/InjectorCLIx64.exe")) {
			const auto pid = GetCurrentProcessId();

			char command_line[MAX_PATH];
			sprintf_s(command_line, MAX_PATH, "ScyllaHide/InjectorCLIx64.exe pid:%d HookLibraryx64.dll nowait", pid);

			STARTUPINFO si = {};
			PROCESS_INFORMATION pi = {};

			bool result = CreateProcessA(
				"ScyllaHide/InjectorCLIx64.exe",
				command_line,
				nullptr,
				nullptr,
				false,
				0,
				nullptr,
				"ScyllaHide",
				&si,
				&pi);

			if (result) {
				LogInfo("Launched Scyllahide process...");

				// Wait for it to finish
				DWORD res = WaitForSingleObject(pi.hProcess, 2000);
				if (res == WAIT_TIMEOUT) {
					LogInfo("Scyllahide process timed out...");
				}

				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);

				LogInfo("Successfully injected ScyllaHide...");
			}
			else {
				LogInfo("Failed launching ScyllaHide process: {}", GetLastError());
			}
		}
		else {
			LogInfo("Could not find ScyllaHide in game directory...");
		}

		return Trampoline(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
	}
};

std::vector<DetourEntry> GetDebugDetours() {
	return { DetourHelper<DetourMainForScyllaHide>::GetDetourEntry("WinMain") };
}
