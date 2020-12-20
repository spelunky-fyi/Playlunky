#include "util.h"
#include "detour/detour.h"

#include <thread>

// Dummy export
__declspec(dllexport) int __stdcall _(void) { return 0; }

static HMODULE s_Spel2Handle = nullptr;

BOOL WINAPI DllMain([[maybe_unused]] HINSTANCE hinst, DWORD dwReason, [[maybe_unused]] LPVOID reserved)
{
	if (s_Spel2Handle == nullptr)
	{
		s_Spel2Handle = GetModuleHandleA("Spel2.exe");
	}

	if (dwReason == DLL_PROCESS_ATTACH) {
		Attach();
	}
	else if (dwReason == DLL_PROCESS_DETACH) {
		Detach();
	}

	return TRUE;
}
