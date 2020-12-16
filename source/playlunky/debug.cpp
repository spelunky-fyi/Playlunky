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
	inline static SigScan::Function<void(__stdcall*)(const char* argv)> Trampoline{
		.Signature = "\x40\x53\x48\x83\xec\x30\x48\x8b\xd9\x48\x8d\x4c\x24\x50\xe8\x2a\x2a\x2a\x2a\x90\xff\x2a\x2a\x2a\x2a\x2a"
	};
	static void Detour(const char* argv)
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

		Trampoline(argv);
	}
};

std::vector<DetourEntry> GetDebugDetours() {
	return { DetourHelper<DetourMainForScyllaHide>::GetDetourEntry("Main") };
}
