#include "win_main.h"

#include "detour_entry.h"
#include "detour_helper.h"
#include "imgui.h"
#include "sigfun.h"
#include "sigscan.h"
#include "util/call_once.h"

#include "../playlunky.h"
#include "log.h"

#include <Windows.h>
#include <string_view>

struct DetourWinMain
{
    inline static SigScan::Function<int(__stdcall*)(HINSTANCE, HINSTANCE, LPSTR, int)> Trampoline{
        .Signature = "\x40\x53\x48\x83\xec\x20\x49\x8b\xd8\xff\x15\x2a\x2a\x2a\x2a\x48\x8b\xc8\xba\x02\x00\x00\x00"_sig
    };
    static int Detour(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
    {
        return Trampoline(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
    }
};

// This is the first function called in WinMain() after the global log is initialized
struct DetourGetGameApi
{
    inline static SigScan::Function<void*(__stdcall*)()> Trampoline{
        .Signature = "\x40\x53\x48\x83\xec\x20\x48\x8b\x05\x2a\x2a\x2a\x2a\x48\x85\xc0\x75\x76\x48\x39\x05\x2a\x2a\x2a\x2a"_sig
    };
    static void* Detour()
    {
        CallOnce([]()
                 { Playlunky::Get().Init(); });
        return Trampoline();
    }
};

// This is the last function called during game initialization
struct DetourGameInitFinalize
{
    inline static SigScan::Function<void*(__stdcall*)(void*, void*, void*)> Trampoline{
        .Signature = "\x48\x89\x5c\x24\x18\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xec\x30\x48\x8b\xf1\x80\x39\x00\x74\x3a\x48\x8b\x0d\x2a\x2a\x2a\x2a\x48\x8b\x01"_sig
    };
    static void* Detour(void* game_ptr, void* other_ptr, void* some_func_ptr)
    {
        void* res = Trampoline(game_ptr, other_ptr, some_func_ptr);
        CallOnce([]()
                 {
                     void* api = DetourGetGameApi::Trampoline();
                     auto decode_imm = [](void* instruction_addr) -> std::size_t
                     { return *(std::uint32_t*)((char*)instruction_addr + 3); };
                     auto swapchain_offset = decode_imm((char*)SigScan::FindPattern("Spel2.exe", "\xba\xf0\xff\xff\xff\x41\xB8\x00\x00\x00\x90"_sig, true) + 17);
                     auto renderer = *(void**)((char*)api + 0x10);
                     [[maybe_unused]] auto api_off = SigScan::GetOffset(api);
                     SetSwapchain(*(void**)((char*)renderer + swapchain_offset));
                 });
        Playlunky::Get().PostGameInit();
        return res;
    }
};

std::vector<struct DetourEntry> GetMainDetours()
{
    return {
        DetourHelper<DetourWinMain>::GetDetourEntry("WinMain"),
        DetourHelper<DetourGetGameApi>::GetDetourEntry("GetGameApi"),
        DetourHelper<DetourGameInitFinalize>::GetDetourEntry("GameInitFinalize")
    };
}
