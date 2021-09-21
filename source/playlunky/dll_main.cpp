#include "playlunky.h"

#include "detour/sigscan.h"
#include "util/format.h"

#include <Windows.h>

// Dummy export
__declspec(dllexport) int __stdcall _(void)
{
    return 0;
}

inline constexpr std::string_view s_SupportedSpelunkyVersion = "1.21.0c";

BOOL WINAPI DllMain([[maybe_unused]] HINSTANCE hinst, DWORD dwReason, [[maybe_unused]] LPVOID reserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        using namespace std::string_view_literals;
        if (void* version_address = SigScan::FindPattern("Spel2.exe", "1.**.**"sv, false))
        {
            const char* version_string = static_cast<const char*>(version_address);
            if (version_string != s_SupportedSpelunkyVersion)
            {
                auto message = fmt::format("This version of Spelunky 2 is not supported by Playlunky, "
                                           "please update your game or check if there is a new version of Playlunky available.\n"
                                           "\tSpelunky Version: {}\n"
                                           "\tSupported Version: {}\n"
                                           "A new version will be released eventually that removes this warning.\n"
                                           "Playlunky might still function correctly, but don't complain if it doesn't.\n"
                                           "Press OK to start without Playlunky or Cancel to yolo.",
                                           version_string,
                                           s_SupportedSpelunkyVersion);
                if (MessageBox(NULL, message.data(), NULL, MB_OKCANCEL) == IDOK)
                {
                    return TRUE;
                }
            }
        }
        else
        {
            auto message = fmt::format("This version of Spelunky 2 is not supported by Playlunky, "
                                       "please update your game or check if there is a new version of Playlunky available.\n"
                                       "\tSpelunky Version: could not be verified\n"
                                       "\tSupported Version: {}\n"
                                       "A new version will be released eventually that removes this warning.\n"
                                       "Playlunky might still function correctly, but don't complain if it doesn't.\n"
                                       "Press OK to start without Playlunky or Cancel to yolo.",
                                       s_SupportedSpelunkyVersion);
            if (MessageBox(NULL, message.data(), NULL, MB_OKCANCEL) == IDOK)
            {
                return TRUE;
            }
        }

        HMODULE game_handle = GetModuleHandle("Spel2.exe");
        Playlunky::Create(game_handle);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        Playlunky::Destroy();
    }

    return TRUE;
}
