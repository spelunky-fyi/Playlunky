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

static VirtualFilesystem* s_Vfs{ nullptr };

struct DetourReadEncrypedFile {
	inline static SigScan::Function<void* (__stdcall*)(const char*, void* (*)(size_t size))> Trampoline{
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
};

enum FMOD_RESULT {
	FMOD_OK,
	FMOD_ERR_UNSUPPORTED = 68
};
struct DetourFmodSystemLoadBankMemory {
	inline static SigScan::Function<int(__stdcall*)(void*, const char*, int, int, int, void**)> Trampoline{
		.Signature = "\x40\x53\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xec\x68\x01\x00\x00\x48\x8b\x05\x2a\x2a\x2a\x2a\x48\x33\xc4\x48\x89\x84\x24\x50\x01\x00\x00\x4c\x8b\xb4\x24\xd8\x01\x00\x00"_sig,
		.Module = "fmodstudio.dll"
	};
	static int Detour(
		[[maybe_unused]] void* fmod_system,
		const char* buffer,
		int length,
		int mode,
		int flags,
		void** bank) {

		s_LastBuffer = buffer;
		s_LastLength = length;
		s_LastMode = mode;
		s_LastFlags = flags;

		*bank = nullptr;
		return FMOD_ERR_UNSUPPORTED;
	}

	static int DoLastLoad(void* fmod_system, void** bank) {
		if (s_LastBuffer != nullptr) {
			const char* buffer{ nullptr };
			int length{ 0 };
			int mode{ 0 };
			int flags{ 0 };

			std::swap(buffer, s_LastBuffer);
			std::swap(length, s_LastLength);
			std::swap(mode, s_LastMode);
			std::swap(flags, s_LastFlags);

			return Trampoline(fmod_system, buffer, length, mode, flags, bank);
		}

		return FMOD_ERR_UNSUPPORTED;
	}

	static inline const char* s_LastBuffer{ nullptr };
	static inline int s_LastLength{ 0 };
	static inline int s_LastMode{ 0 };
	static inline int s_LastFlags{ 0 };
};

struct DetourFmodSystemLoadBankFile {
	inline static SigScan::Function<int(__stdcall*)(void*, const char*, int, void**)> Trampoline{
		.Signature = "\x40\x53\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xec\x68\x01\x00\x00\x48\x8b\x05\x2a\x2a\x2a\x2a\x48\x33\xc4\x48\x89\x84\x24\x50\x01\x00\x00\x44\x89\x44\x24\x30"_sig,
		.Module = "fmodstudio.dll"
	};
	static int Detour(void* fmod_system, const char* file_name, int flags, void** bank) {
		if (const auto file_path = s_Vfs->GetFilePath(file_name)) {
			const std::string file_path_string = file_path.value().string();
			const int result = Trampoline(fmod_system, file_path_string.c_str(), flags, bank);
			if (result == FMOD_OK) {
				return FMOD_OK;
			}
		}

		const int last_memory_result = DetourFmodSystemLoadBankMemory::DoLastLoad(fmod_system, bank);
		if (last_memory_result == FMOD_OK) {
			return FMOD_OK;
		}

		return Trampoline(fmod_system, file_name, flags, bank);
	}
};

std::vector<DetourEntry> GetFileIODetours() {
	const bool random_char_select = []() {
		return INIReader("playlunky.ini").GetBoolean("settings", "random_character_select", false);
	}();
	if (random_char_select) {
		srand(static_cast<unsigned int>(time(nullptr))); // use something better for randomness?
		return {
			DetourHelper<DetourReadEncrypedFileWithCharacterRandomizer>::GetDetourEntry("ReadEncrypedFile"),
			DetourHelper<DetourFmodSystemLoadBankMemory>::GetDetourEntry("FMOD::System::loadBankMemory"),
			DetourHelper<DetourFmodSystemLoadBankFile>::GetDetourEntry("FMOD::System::loadBankFile")
		};
	}
	else {
		return {
			DetourHelper<DetourReadEncrypedFile>::GetDetourEntry("ReadEncrypedFile"),
			DetourHelper<DetourFmodSystemLoadBankMemory>::GetDetourEntry("FMOD::System::loadBankMemory"),
			DetourHelper<DetourFmodSystemLoadBankFile>::GetDetourEntry("FMOD::System::loadBankFile")
		};
	}
}

void SetVfs(VirtualFilesystem* vfs) {
	s_Vfs = vfs;
}
