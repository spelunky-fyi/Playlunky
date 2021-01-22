#include "debug.h"

#include "detour_entry.h"
#include "detour_helper.h"
#include "sigscan.h"
#include "log.h"

#include "mod/virtual_filesystem.h"

#include <cstdint>

struct DetourReadEncrypedFile {
	inline static SigScan::Function<void*(__stdcall*)(const char*, void* (*)(size_t size))> Trampoline{
		.Signature = "\x48\x8b\xc4\x48\x89\x58\x18\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8d\xa8\x38\xff\xff\xff"_sig
	};
	static void* Detour(const char* file_path, void* (*alloc_fun)(size_t size))
	{
		if (s_Vfs) {
			if (auto* file_info = s_Vfs->LoadFile(file_path, alloc_fun)) {
				return file_info;
			}
		}

		return Trampoline(file_path, alloc_fun);
	}

	inline static VirtualFilesystem* s_Vfs{ nullptr };
};

std::vector<DetourEntry> GetFileIODetours() {
	return { DetourHelper<DetourReadEncrypedFile>::GetDetourEntry("ReadEncrypedFile") };
}

void SetVfs(VirtualFilesystem* vfs) {
	DetourReadEncrypedFile::s_Vfs = vfs;
}
