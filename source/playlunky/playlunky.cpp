#include <playlunky.h>

#include "log.h"
#include "mod/mod_manager.h"
#include "mod/virtual_filesystem.h"
#include "detour/detour.h"
#include "detour/file_io.h"
#include "detour/fmod_crap.h"

struct Playlunky::PlaylunkyImpl {
	HMODULE GameModule;
	std::unique_ptr<VirtualFilesystem> Vfs;
	std::unique_ptr<ModManager> Manager;
};

struct PlaylunkyDeleter {
	void operator()(Playlunky* playlunky) {
		delete playlunky;
	}
};
static std::unique_ptr<Playlunky, PlaylunkyDeleter> s_PlaylunkyInstance;

Playlunky& Playlunky::Get() {
	if (s_PlaylunkyInstance == nullptr) {
		LogFatal("Playlunky::Get() called without a valid instance present");
	}

	return *s_PlaylunkyInstance;
}

void Playlunky::Create(HMODULE game_module) {
	if (s_PlaylunkyInstance != nullptr) {
		LogFatal("Playlunky::Create() with a valid instance already present");
		return;
	}

	s_PlaylunkyInstance = { new Playlunky(game_module), PlaylunkyDeleter{} };
}
void Playlunky::Destroy() {
	if (s_PlaylunkyInstance == nullptr) {
		LogFatal("Playlunky::Destroy() called without a valid instance present");
		return;
	}

	s_PlaylunkyInstance.reset();
}

void Playlunky::Init() {
	LogInfo("Initializing Playlunky...");
	LogInfo("Playlunky Version: " PLAYLUNKY_VERSION);

	mImpl->Vfs = std::make_unique<VirtualFilesystem>();
	mImpl->Manager = std::make_unique<ModManager>("Mods/Packs", *mImpl->Vfs);

	SetFileIOVfs(mImpl->Vfs.get());
	SetFmodVfs(mImpl->Vfs.get());
}

void Playlunky::PostGameInit() {
	LogInfo("Finalizing Playlunky setup...");
	
	mImpl->Manager->PostGameInit();
}

Playlunky::Playlunky(HMODULE game_module)
	: mImpl{ new PlaylunkyImpl{ game_module } }
{
	Attach();
}

Playlunky::~Playlunky() {
	Detach();
}
