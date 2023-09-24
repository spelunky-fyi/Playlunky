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

std::int64_t g_ExtraPipesTextureId{};
using SetupPipes = void(Entity*);
SetupPipes* g_SetupPipesTrampoline{ nullptr };
SpelunkyScript* g_ExtraPipesScript{ nullptr };

using GetNeighbouringGridEntityOfSameType = uint32_t(Entity* thorns, float offset_x, float offset_y);
GetNeighbouringGridEntityOfSameType* g_GetNeighbouringGridEntityOfSameType{ nullptr };

bool g_OutOfBoundsLiquids{ false };

struct PackedResource
{
    PackedResource(LPSTR resource, LPSTR resource_type)
    {
        HMODULE this_module = GetModuleHandle("playlunky64.dll");
        if (HRSRC png_resource = FindResource(this_module, resource, resource_type))
        {
            if (HGLOBAL png_data = LoadResource(this_module, png_resource))
            {
                handle = png_data;

                DWORD png_size = SizeofResource(this_module, png_resource);
                data = { (std::uint8_t*)LockResource(png_data), png_size };
            }
        }
    }
    virtual ~PackedResource()
    {
        UnlockResource(handle);
    }

    HGLOBAL handle{ NULL };
    std::span<std::uint8_t> data;
};
struct PngResource : PackedResource
{
    PngResource(LPSTR resource)
        : PackedResource{ resource, MAKEINTRESOURCE(PNG_FILE) }
    {
        if (handle)
        {
            image.Load(data);
        }
    }

    Image image;
};
struct TextResource : PackedResource
{
    TextResource(LPSTR resource)
        : PackedResource{ resource, MAKEINTRESOURCE(TEXT_FILE) }
    {
        if (handle)
        {
            text = { reinterpret_cast<const char*>(data.data()), data.size() };
        }
    }

    std::string_view text;
};

void SetupMissingThorns(Entity* thorns)
{
    g_SetupThornsTrampoline(thorns);
    const auto current_texture_tile = SpelunkyEntity_GetTextureTile(thorns);
    if (current_texture_tile == 127)
    {
        const auto left = g_GetNeighbouringGridEntityOfSameType(thorns, 0.0f, 1.0f) & 0x1;
        const auto right = g_GetNeighbouringGridEntityOfSameType(thorns, 0.0f, -1.0f) & 0x1;
        const auto top = g_GetNeighbouringGridEntityOfSameType(thorns, 1.0f, 0.0f) & 0x1;
        const auto bottom = g_GetNeighbouringGridEntityOfSameType(thorns, -1.0f, 0.0f) & 0x1;
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
}
void SetupMissingPipes(Entity* pipe)
{
    g_SetupPipesTrampoline(pipe);
    if (SpelunkyEntity_GetTexture(pipe) == g_ExtraPipesTextureId)
    {
        SpelunkyEntity_SetTextureTile(pipe, 3);
    }
    else if (SpelunkyEntity_GetTextureTile(pipe) == 127)
    {
        const auto left = g_GetNeighbouringGridEntityOfSameType(pipe, 0.0f, 1.0f) & 0x1;
        const auto right = g_GetNeighbouringGridEntityOfSameType(pipe, 0.0f, -1.0f) & 0x1;
        const auto top = g_GetNeighbouringGridEntityOfSameType(pipe, 1.0f, 0.0f) & 0x1;
        const auto bottom = g_GetNeighbouringGridEntityOfSameType(pipe, -1.0f, 0.0f) & 0x1;
        const auto mask = left | (right << 1) | (top << 2) | (bottom << 3);
        const auto texture_tile = [mask]() -> uint16_t
        {
            switch (mask)
            {
            case 0b1111:
                return 0;
            case 0b0111:
                return 1;
            case 0b1011:
                return 2;
            case 0b1101:
                return 4;
            case 0b1110:
                return 5;
            default:
                LogError("Pipes have missing textures but valid setup... why?");
                [[fallthrough]];
            case 0b0000:
                return 127;
            }
        }();
        if (texture_tile != 127)
        {
            SpelunkyEntity_SetTexture(pipe, g_ExtraPipesTextureId);
            SpelunkyEntity_SetTextureTile(pipe, texture_tile);
        }
    }
}

void BugFixesMount(VirtualFilesystem& vfs,
                   const std::filesystem::path& db_folder)
{
    const auto bug_fixes_folder = db_folder / "Mods/BugFixes";
    vfs.MountFolder(bug_fixes_folder.string(), std::numeric_limits<int64_t>::max(), VfsType::Backend);
}
bool BugFixesInit(const PlaylunkySettings& settings,
                  const std::filesystem::path& db_folder,
                  const std::filesystem::path& original_data_folder)
{
    namespace fs = std::filesystem;

    static constexpr auto decode_call = [](void* addr) -> void*
    {
        return (char*)addr + (*(int32_t*)((char*)addr + 1)) + 5;
    };

    const auto bug_fixes_folder = db_folder / "Mods/BugFixes";

    if (settings.GetBool("bug_fixes", "missing_thorns", true))
    {
        const auto extra_thorns_dds_path = bug_fixes_folder / "Data/Textures/extra_thorns.DDS";
        if (!fs::exists(extra_thorns_dds_path))
        {
            const auto extra_thorns_path = original_data_folder / "Data/Textures/extra_thorns.png";

            PngResource extra_thorns_png{ MAKEINTRESOURCE(EXTRA_THORNS) };
            extra_thorns_png.image.Write(extra_thorns_path);
            extra_thorns_png.image.Write("Mods/Extracted/Data/Textures/extra_thorns.png");

            if (!ConvertRBGAToDds(extra_thorns_png.image.GetData(), extra_thorns_png.image.GetWidth(), extra_thorns_png.image.GetHeight(), extra_thorns_dds_path))
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

        constexpr std::string_view g_SetupThornsFuncPattern{ "\x34\x01\xc0\xe0\x03\x08\xc8" };
        g_SetupThornsTrampoline = static_cast<SetupThorns*>(SigScan::FindFunctionStart(SigScan::FindPattern("Spel2.exe", g_SetupThornsFuncPattern, true)));
        g_GetNeighbouringGridEntityOfSameType = static_cast<GetNeighbouringGridEntityOfSameType*>(decode_call(SigScan::FindPattern("\xe8", g_SetupThornsTrampoline, (char*)g_SetupThornsTrampoline + 0x1000)));

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        DetourAttach((void**)&g_SetupThornsTrampoline, SetupMissingThorns);

        const LONG error = DetourTransactionCommit();
        if (error != NO_ERROR)
        {
            LogError("Could not fix thorns textures: {}", error);
        }
    }

    if (settings.GetBool("bug_fixes", "missing_pipes", false))
    {
        const auto extra_pipes_dds_path = bug_fixes_folder / "Data/Textures/extra_pipes.DDS";
        if (!fs::exists(extra_pipes_dds_path))
        {
            const auto extra_pipes_path = original_data_folder / "Data/Textures/extra_pipes.png";

            PngResource extra_pipes_png{ MAKEINTRESOURCE(EXTRA_PIPES) };
            extra_pipes_png.image.Write(extra_pipes_path);
            extra_pipes_png.image.Write("Mods/Extracted/Data/Textures/extra_pipes.png");

            if (!ConvertRBGAToDds(extra_pipes_png.image.GetData(), extra_pipes_png.image.GetWidth(), extra_pipes_png.image.GetHeight(), extra_pipes_dds_path))
            {
                return false;
            }
        }

        Spelunky_TextureDefinition extra_pipes_texture_def{
            "Data/Textures/extra_pipes.DDS",
            128 * 3,
            128 * 2,
            128,
            128,
        };
        g_ExtraPipesTextureId = Spelunky_DefineTexture(extra_pipes_texture_def);

        const auto extra_pipes_script_path = bug_fixes_folder / "extra_pipes.lua";
        if (!fs::exists(extra_pipes_script_path))
        {
            if (auto out = std::ofstream{ extra_pipes_script_path })
            {
                TextResource extra_pipes_png{ MAKEINTRESOURCE(EXTRA_PIPES_SCRIPT) };
                out << extra_pipes_png.text;
            }
        }
        g_ExtraPipesScript = Spelunky_CreateScript(extra_pipes_script_path.string().c_str(), true);

        constexpr std::string_view g_SetupPipesFuncPattern{ "\x88\x86\xa1\x00\x00\x00\x8d\x48\xfd" };
        g_SetupPipesTrampoline = static_cast<SetupPipes*>(SigScan::FindFunctionStart(SigScan::FindPattern("Spel2.exe", g_SetupPipesFuncPattern, true)));
        if (g_GetNeighbouringGridEntityOfSameType == nullptr)
        {
            g_GetNeighbouringGridEntityOfSameType = static_cast<GetNeighbouringGridEntityOfSameType*>(decode_call(SigScan::FindPattern("\xe8", g_SetupPipesTrampoline, (char*)g_SetupThornsTrampoline + 0x1000)));
        }
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        DetourAttach((void**)&g_SetupPipesTrampoline, SetupMissingPipes);

        const LONG error = DetourTransactionCommit();
        if (error != NO_ERROR)
        {
            LogError("Could not fix pipes: {}", error);
        }
    }

    g_OutOfBoundsLiquids = settings.GetBool("bug_fixes", "out_of_bounds_liquids", true);

    return true;
}
void BugFixesCleanup()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (g_SetupThornsTrampoline)
    {
        DetourDetach((void**)&g_SetupThornsTrampoline, SetupMissingThorns);
    }
    if (g_SetupPipesTrampoline)
    {
        DetourDetach((void**)&g_SetupPipesTrampoline, SetupMissingPipes);
        Spelunky_FreeScript(g_ExtraPipesScript);
    }

    const LONG error = DetourTransactionCommit();
    if (error != NO_ERROR)
    {
        LogError("Could not fix pipes: {}", error);
    }
}
