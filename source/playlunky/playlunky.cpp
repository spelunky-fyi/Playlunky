#include <playlunky.h>

#include "detour/detour.h"
#include "detour/fmod_crap.h"
#include "log.h"
#include "mod/mod_manager.h"
#include "mod/save_game.h"
#include "mod/virtual_filesystem.h"
#include "playlunky_settings.h"
#include "version.h"

#include <Windows.h>

struct Playlunky::PlaylunkyImpl
{
    HMODULE GameModule;
    PlaylunkySettings Settings;
    std::unique_ptr<VirtualFilesystem> Vfs;
    std::unique_ptr<ModManager> Manager;
};

struct PlaylunkyDeleter
{
    void operator()(Playlunky* playlunky)
    {
        delete playlunky;
    }
};
static std::unique_ptr<Playlunky, PlaylunkyDeleter> s_PlaylunkyInstance;

Playlunky& Playlunky::Get()
{
    if (s_PlaylunkyInstance == nullptr)
    {
        LogFatal("Playlunky::Get() called without a valid instance present");
    }

    return *s_PlaylunkyInstance;
}

void Playlunky::Create(void* game_module)
{
    if (s_PlaylunkyInstance != nullptr)
    {
        LogFatal("Playlunky::Create() with a valid instance already present");
        return;
    }

    s_PlaylunkyInstance = { new Playlunky{ (HMODULE)game_module }, PlaylunkyDeleter{} };
}
void Playlunky::Destroy()
{
    if (s_PlaylunkyInstance == nullptr)
    {
        LogFatal("Playlunky::Destroy() called without a valid instance present");
        return;
    }

    s_PlaylunkyInstance.reset();
}
bool Playlunky::IsDestroy()
{
    return s_PlaylunkyInstance == nullptr;
}

void Playlunky::Init()
{
    LogInfo("Initializing Playlunky...");
    LogInfo("Playlunky Version: {}", playlunky_version());

    mImpl->Vfs = std::make_unique<VirtualFilesystem>();
    mImpl->Manager = std::make_unique<ModManager>("Mods/Packs", mImpl->Settings, *mImpl->Vfs);

    SetFmodVfs(mImpl->Vfs.get());
    SetSaveGameVfs(mImpl->Vfs.get());
}

void Playlunky::PostGameInit()
{
    LogInfo("Finalizing Playlunky setup...");

    mImpl->Manager->PostGameInit(mImpl->Settings);
}

const PlaylunkySettings& Playlunky::GetSettings() const
{
    return mImpl->Settings;
}

Playlunky::Playlunky(void* game_module)
    : mImpl{ new PlaylunkyImpl{ .GameModule{ (HMODULE)game_module }, .Settings{ "playlunky.ini" } } }
{
    Attach(mImpl->Settings);
}

Playlunky::~Playlunky()
{
    Detach(mImpl->Settings);
}
