#include "bug_fixes.h"

#include "../../res/resource_playlunky64.h"
#include "dds_conversion.h"
#include "log.h"
#include "playlunky_settings.h"
#include "util/image.h"
#include "util/on_scope_exit.h"
#include "virtual_filesystem.h"

#include "detour/sigscan.h"

#include <Windows.h>
#include <detours.h>
#include <spel2.h>

std::int64_t g_ExtraThornsTextureId{};
using SetupThorns = void(Entity*);
SetupThorns* g_SetupThornsTrampoline{ nullptr };
using GetNeighbouringThorns = uint32_t(Entity* thorns, float offset_x, float offset_y);
GetNeighbouringThorns* g_GetNeighbouringThorns{ nullptr };

bool InitBugFixes(VirtualFilesystem& /*vfs*/,
                  const PlaylunkySettings& settings,
                  const std::filesystem::path& db_folder,
                  const std::filesystem::path& original_data_folder)
{
    namespace fs = std::filesystem;

    // Extract extra thorns
    if (settings.GetBool("bug_fixes", "missing_thorns", true))
    {
        const auto extra_thorns_path = original_data_folder / "Data/Textures/extra_thorns.png";
        if (!fs::exists(extra_thorns_path))
        {
            auto acquire_png_resource = [](LPSTR resource)
            {
                HMODULE this_module = GetModuleHandle("playlunky64.dll");
                if (HRSRC png_resource = FindResource(this_module, resource, MAKEINTRESOURCE(PNG_FILE)))
                {
                    if (HGLOBAL png_data = LoadResource(this_module, png_resource))
                    {
                        DWORD png_size = SizeofResource(this_module, png_resource);
                        return std::pair{ png_data, std::span<std::uint8_t>{ (std::uint8_t*)LockResource(png_data), png_size } };
                    }
                }
                return std::pair{ HGLOBAL{ NULL }, std::span<std::uint8_t>{} };
            };
            auto [extra_thorns_res, extra_thorns_png] = acquire_png_resource(MAKEINTRESOURCE(EXTRA_THORNS));
            OnScopeExit release_resources{ [extra_thorns_res]
                                           {
                                               UnlockResource(extra_thorns_res);
                                           } };

            Image extra_thorns;
            extra_thorns.Load(extra_thorns_png);
            extra_thorns.Write(extra_thorns_path);
            extra_thorns.Write("Mods/Extracted/Data/Textures/extra_thorns.png");

            const auto extra_thorns_dds_path = db_folder / "Data/Textures/extra_thorns.DDS";
            if (!ConvertRBGAToDds(extra_thorns.GetData(), extra_thorns.GetWidth(), extra_thorns.GetHeight(), extra_thorns_dds_path))
            {
                return false;
            }
        }

        Spelunky_TextureDefinition extra_thorns_texture_def{
            "Data/Textures/extra_thorns.DDS",
            128 * 3,
            128 * 2,
            128,
            128,
        };
        g_ExtraThornsTextureId = Spelunky_DefineTexture(extra_thorns_texture_def);

        static constexpr auto decode_call = [](void* addr) -> void* {
            return (char*)addr + (*(int32_t*)((char*)addr + 1)) + 5;
        };

        constexpr std::string_view g_SetupThornsFuncPattern{ "\x34\x01\xc0\xe0\x03\x08\xc8" };
        g_SetupThornsTrampoline = static_cast<SetupThorns*>(SigScan::FindFunctionStart(SigScan::FindPattern("Spel2.exe", g_SetupThornsFuncPattern, true)));
        g_GetNeighbouringThorns = static_cast<GetNeighbouringThorns*>(decode_call(SigScan::FindPattern("\xe8", g_SetupThornsTrampoline, (char*)g_SetupThornsTrampoline + 0x1000)));

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        static constexpr auto c_SetupMissingThorns = [](Entity* thorns)
        {
            g_SetupThornsTrampoline(thorns);
            const auto current_texture_tile = SpelunkyEntity_GetTextureTile(thorns);
            if (current_texture_tile == 127)
            {
                const auto left = g_GetNeighbouringThorns(thorns, 0.0f, 1.0f) & 0x1;
                const auto right = g_GetNeighbouringThorns(thorns, 0.0f, -1.0f) & 0x1;
                const auto top = g_GetNeighbouringThorns(thorns, 1.0f, 0.0f) & 0x1;
                const auto bottom = g_GetNeighbouringThorns(thorns, -1.0f, 0.0f) & 0x1;
                const auto mask = left | (right << 1) | (top << 2) | (bottom << 3);
                const auto texture_tile = [mask]() -> uint16_t
                {
                    switch (mask)
                    {
                    case 0b0000:
                        return 0;
                    case 0b1111:
                        return 3;
                    case 0b0111:
                        return 1;
                    case 0b1011:
                        return 2;
                    case 0b1101:
                        return 4;
                    case 0b1110:
                        return 5;
                    default:
                        LogError("Thorns have missing textures but valid setup... why?");
                        return 127;
                    }
                }();
                if (texture_tile != 127)
                {
                    SpelunkyEntity_SetTexture(thorns, g_ExtraThornsTextureId);
                    SpelunkyEntity_SetTextureTile(thorns, texture_tile);
                }
            }
        };
        DetourAttach((void**)&g_SetupThornsTrampoline, (SetupThorns*)c_SetupMissingThorns);

        const LONG error = DetourTransactionCommit();
        if (error != NO_ERROR)
        {
            LogError("Could not fix thorns textures: {}", error);
        }
    }

    return true;
}
