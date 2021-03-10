#include "debug.h"

#include "detour_entry.h"
#include "detour_helper.h"
#include "sigfun.h"
#include "log.h"

#include <Windows.h>
#include <cstdint>
#include <filesystem>

struct DetourIsDebuggerPresent
{
	inline static auto Trampoline = &IsDebuggerPresent;
	static BOOL Detour()
	{
		static bool s_AttemptedScyllaHideInject = false;
		if (s_AttemptedScyllaHideInject) {
			s_AttemptedScyllaHideInject = true;

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
						LogError("Scyllahide process timed out...");
					}

					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);

					LogInfo("Successfully injected ScyllaHide...");
				}
				else {
					LogError("Failed launching ScyllaHide process: {}", GetLastError());
				}
			}
			else {
				LogError("Could not find ScyllaHide in game directory...");
			}
		}

		return FALSE;
	}
};

std::vector<DetourEntry> GetDebugDetours() {
	return { DetourHelper<DetourIsDebuggerPresent>::GetDetourEntry("IsDebuggerPresent") };
}
