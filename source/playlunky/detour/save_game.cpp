#include "save_game.h"

#include "detour_entry.h"
#include "detour_helper.h"
#include "log.h"
#include "mod/virtual_filesystem.h"
#include "playlunky_settings.h"
#include "sigscan.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

static VirtualFilesystem* s_SaveGameVfs{ nullptr };

struct DetourLoadFileFromDisk
{
    inline static SigScan::Function<void(__stdcall*)(const char*, void**, size_t*)> Trampoline{
        .Signature = "\x41\x57\x41\x56\x56\x57\x53\x48\x81\xec\x20\x01\x00\x00\x4c\x89\xc3\x49\x89\xd7\x49\x89\xce"_sig,
        .FindFunctionStart = false,
    };
    static void Detour(const char* save_game_sav, void** out_buf, size_t* out_size)
    {
        using namespace std::string_view_literals;
        if (save_game_sav == "savegame.sav"sv)
        {
            if (const auto sav_replacement = s_SaveGameVfs->GetDifferentFilePath(save_game_sav))
            {
                const std::string sav_replacement_str = sav_replacement.value().string();
                Trampoline(sav_replacement_str.c_str(), out_buf, out_size);
                return;
            }
        }
        Trampoline(save_game_sav, out_buf, out_size);
    }
};

struct DetourStoreSaveGame
{
    inline static SigScan::Function<void(__stdcall*)(const char*, const char*, void*, size_t)> Trampoline{
        .Signature = "\x41\x57\x41\x56\x56\x57\x55\x53\x48\x81\xec\x38\x01\x00\x00\x4c\x89\xcb\x4c\x89\xc6"_sig,
        .FindFunctionStart = false,
    };
    static void Detour(const char* save_game_bak, const char* save_game_sav, void* buf, size_t size)
    {
        using namespace std::string_view_literals;
        if (save_game_sav == "savegame.sav"sv)
        {
            if (s_Block)
            {
                return;
            }

            if (s_AllowMod)
            {
                if (const auto sav_replacement = s_SaveGameVfs->GetDifferentFilePath(save_game_sav))
                {
                    const std::string sav_replacement_str = sav_replacement.value().string();
                    const auto bak_replacement_str = std::filesystem::path{ sav_replacement.value() }.replace_extension(".bak").string();
                    Trampoline(bak_replacement_str.c_str(), sav_replacement_str.c_str(), buf, size);
                    return;
                }
            }
        }
        Trampoline(save_game_bak,  save_game_sav, buf, size);
    }

    inline static bool s_Block{ false };
    inline static bool s_AllowMod{ false };
};

std::vector<DetourEntry> GetSaveGameDetours(const PlaylunkySettings& settings)
{
    const bool block_save_game = settings.GetBool("general_settings", "block_save_game", false);
    const bool allow_save_game_mods = settings.GetBool("general_settings", "allow_save_game_mods", true);
    if (allow_save_game_mods || block_save_game)
    {
        DetourStoreSaveGame::s_Block = block_save_game;
        DetourStoreSaveGame::s_AllowMod = allow_save_game_mods;

        if (allow_save_game_mods)
        {
            return {
                DetourHelper<DetourLoadFileFromDisk>::GetDetourEntry("LoadSaveGame"),
                DetourHelper<DetourStoreSaveGame>::GetDetourEntry("StoreSaveGame")
            };
        }
        else
        {
            return {
                DetourHelper<DetourStoreSaveGame>::GetDetourEntry("StoreSaveGame")
            };
        }
    }
    return {};
}

void SetSaveGameVfs(VirtualFilesystem* vfs)
{
    s_SaveGameVfs = vfs;
}
