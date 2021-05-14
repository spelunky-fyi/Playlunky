#include "playlunky.h"

// Dummy export
__declspec(dllexport) int __stdcall _(void)
{
    return 0;
}

BOOL WINAPI DllMain([[maybe_unused]] HINSTANCE hinst, DWORD dwReason, [[maybe_unused]] LPVOID reserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        HMODULE game_handle = GetModuleHandle("Spel2.exe");
        Playlunky::Create(game_handle);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        Playlunky::Destroy();
    }

    return TRUE;
}
