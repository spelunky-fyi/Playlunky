#include "file_io.h"

#include "detour_entry.h"
#include "detour_helper.h"
#include "sigscan.h"
#include "log.h"
#include "mod/virtual_filesystem.h"
#include "util/regex.h"

#include <cstdint>

static VirtualFilesystem* s_FileIOVfs{ nullptr };

struct DetourReadEncrypedFile {
	inline static SigScan::Function<void* (__stdcall*)(const char*, void* (*)(size_t size))> Trampoline{
		.Signature = "\x48\x8b\xc4\x48\x89\x58\x18\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8d\x6c\x24\x80\x48\x81\xec\x80\x01\x00\x00"_sig
		
	};
	static void* Detour(const char* file_path, void* (*alloc_fun)(size_t size))
	{
		if (s_FileIOVfs) {
			if (auto* file_info = s_FileIOVfs->LoadFile(file_path, alloc_fun)) {
				return file_info;
			}
		}

		return Trampoline(file_path, alloc_fun);
	}
};

std::vector<DetourEntry> GetFileIODetours() {
	return {
		DetourHelper<DetourReadEncrypedFile>::GetDetourEntry("ReadEncrypedFile")
	};
}

void SetFileIOVfs(VirtualFilesystem* vfs) {
	s_FileIOVfs = vfs;
}
