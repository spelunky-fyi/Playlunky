#include "debug.h"

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

struct DetourReadEncrypedFileWithCharacterRandomizer {
	inline static SigScan::Function<void* (__stdcall*)(const char*, void* (*)(size_t size))> Trampoline{
		.Signature = "\x48\x8b\xc4\x48\x89\x58\x18\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8d\xa8\x38\xff\xff\xff"_sig
	};
	static void* Detour(const char* file_path, void* (*alloc_fun)(size_t size))
	{
		if (s_Vfs) {
			if (ctre::match<s_CharacterRule>(file_path)) {
				const auto all_files = s_Vfs->GetAllFilePaths(file_path);
				if (!all_files.empty()) {
					const auto random_file = all_files[rand() % all_files.size()]; // use something better for randomness?
					const auto random_file_str = random_file.string();
					if (auto* file_info = s_Vfs->LoadSpecificFile(random_file_str.c_str(), alloc_fun)) {
						return file_info;
					}
				}
			}
			else {
				if (auto* file_info = s_Vfs->LoadFile(file_path, alloc_fun)) {
					return file_info;
				}
			}
		}

		return Trampoline(file_path, alloc_fun);
	}

	inline static VirtualFilesystem* s_Vfs{ nullptr };
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

void SetVfs(VirtualFilesystem* vfs) {
	DetourReadEncrypedFile::s_Vfs = vfs;
	DetourReadEncrypedFileWithCharacterRandomizer::s_Vfs = vfs;
}
