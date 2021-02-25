#include "file_io.h"

#include "detour_entry.h"
#include "detour_helper.h"
#include "sigscan.h"
#include "log.h"
#include "mod/virtual_filesystem.h"
#include "util/regex.h"

#include <cstdint>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4244)
#include <INIReader.h>
#pragma warning(pop)

static constexpr ctll::fixed_string s_CharacterRule{ ".+char_(.*)\\.DDS" };

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

struct DetourReadEncrypedFileWithCharacterRandomizer {
	inline static SigScan::Function<void* (__stdcall*)(const char*, void* (*)(size_t size))> Trampoline{
		.Signature = "\x48\x8b\xc4\x48\x89\x58\x18\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8d\xa8\x38\xff\xff\xff"_sig
	};
	static void* Detour(const char* file_path, void* (*alloc_fun)(size_t size))
	{
		if (s_FileIOVfs) {
			if (ctre::match<s_CharacterRule>(file_path)) {
				const auto all_files = s_FileIOVfs->GetAllFilePaths(file_path);
				if (!all_files.empty()) {
					const auto random_file = all_files[rand() % all_files.size()]; // use something better for randomness?
					const auto random_file_str = random_file.string();
					if (auto* file_info = s_FileIOVfs->LoadSpecificFile(random_file_str.c_str(), alloc_fun)) {
						return file_info;
					}
				}
			}
			else {
				if (auto* file_info = s_FileIOVfs->LoadFile(file_path, alloc_fun)) {
					return file_info;
				}
			}
		}

		return Trampoline(file_path, alloc_fun);
	}
};

std::vector<DetourEntry> GetFileIODetours() {
	const bool random_char_select = []() {
		return INIReader("playlunky.ini").GetBoolean("settings", "random_character_select", false);
	}();
	if (random_char_select) {
		srand(static_cast<unsigned int>(time(nullptr))); // use something better for randomness?
		return {
			DetourHelper<DetourReadEncrypedFileWithCharacterRandomizer>::GetDetourEntry("ReadEncrypedFile")
		};
	}
	else {
		return {
			DetourHelper<DetourReadEncrypedFile>::GetDetourEntry("ReadEncrypedFile")
		};
	}
}

void SetFileIOVfs(VirtualFilesystem* vfs) {
	s_FileIOVfs = vfs;
}
