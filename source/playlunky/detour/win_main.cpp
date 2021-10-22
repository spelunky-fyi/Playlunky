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
        .Signature = "\x8b\x45\xfc\x48\x83\xc4\x38\x5e\x5d\xc3"_sig
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
        .Signature = "\x48\x8d\xac\x24\x80\x00\x00\x00\x48\xc7\x85\x70\x08\x00\x00\xfe\xff\xff\xff"_sig
    };
    static void* Detour()
    {
        CallOnce([]()
                 { Playlunky::Get().Init(); });
        return Trampoline();
    }
};

// This is the last function called during game initialization
struct DetourInitGameManager
{
    inline static SigScan::Function<void*(__stdcall*)(void*, void*, void*, void*)> Trampoline{
        .Signature = "\x48\xc7\x85\x18\x04\x00\x00\xfe\xff\xff\xff\x48\x89\x8d\xd0\x03\x00\x00"_sig
    };
    static void* Detour(void* game_ptr, void* other_ptr, void* more_ptr, void* some_func_ptr)
    {
        void* res = Trampoline(game_ptr, other_ptr, more_ptr, some_func_ptr);
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
        DetourHelper<DetourInitGameManager>::GetDetourEntry("GameInitFinalize")
    };
}
