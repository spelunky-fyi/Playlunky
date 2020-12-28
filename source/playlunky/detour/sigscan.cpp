#include "sigscan.h"

#include <Windows.h>
#include <Psapi.h>
#include <winnt.h>

#include <cstdint>
#include <span>

namespace SigScan {
	MODULEINFO GetModuleInfo(HMODULE module) {
		MODULEINFO module_info = { 0 };
		GetModuleInformation(GetCurrentProcess(), module, &module_info, sizeof(MODULEINFO));
		return module_info;
	}

	PIMAGE_NT_HEADERS RtlImageNtHeader(_In_ PVOID Base) {
		static HMODULE ntdll_dll = GetModuleHandleA("ntdll.dll");
		static auto proc = (decltype(RtlImageNtHeader)*)GetProcAddress(ntdll_dll, "RtlImageNtHeader");
		return proc(Base);
	}

	std::span<IMAGE_SECTION_HEADER> GetImageSectionHeaders(HMODULE module) {
		PIMAGE_NT_HEADERS64 nt_headers = RtlImageNtHeader(module);
		WORD num_sections = nt_headers->FileHeader.NumberOfSections;
		PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt_headers);
		return { section, num_sections };
	}

	void* FindPattern(const char* module_name, const char* signature, bool code_only) {
		if (module_name == nullptr || signature == nullptr)
			return 0;

		if (HMODULE module = GetModuleHandleA(module_name)) {
			std::size_t pattern_length = (std::size_t)strlen(signature);

			MODULEINFO module_info = GetModuleInfo(module);
			std::size_t base = (std::size_t)module_info.lpBaseOfDll;

			std::span<IMAGE_SECTION_HEADER> section_headers = GetImageSectionHeaders(module);
			for (auto& section : section_headers) {
				static constexpr auto executable_code_flags = IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE;
				if (!code_only || (section.Characteristics & executable_code_flags) == executable_code_flags) {
					std::size_t size = (std::size_t)section.SizeOfRawData;
					std::size_t section_base = base + (std::size_t)section.VirtualAddress;

					std::size_t section_start = 0;

					// Skip the bundled data for much faster scanning
					if (strstr(module_name, "Spel2.exe") && strcmp((const char*)section.Name, ".text") == 0) {
						while (true) {
							std::size_t asset_size = ((const std::uint32_t*)(section_base + section_start))[0];
							std::size_t name_size = ((const std::uint32_t*)(section_base + section_start))[1];
							if (asset_size == 0 && name_size == 0) {
								break;
							}
							section_start += asset_size + name_size + 2 * sizeof(std::uint32_t);
						};
					}

					for (std::size_t j = section_start; j < size - pattern_length; j++) {
						bool found = true;
						for (std::size_t k = 0; k < pattern_length && found; k++) {
							found = signature[k] == '*' || signature[k] == *(char*)(section_base + j + k);
						}

						if (found) {
							return (void*)(section_base + j);
						}
					}
				}
			}
		}

		return nullptr;
	}

	void* FindPattern(const char* signature, bool code_only) {
		char module_name[MAX_PATH];
		GetModuleFileNameA(0, module_name, MAX_PATH);
		return FindPattern(module_name, signature, code_only);
	}

	void* GetDataSection() {
		char module_name[MAX_PATH];
		GetModuleFileNameA(0, module_name, MAX_PATH);
		if (HMODULE module = GetModuleHandleA(module_name)) {
			MODULEINFO module_info = GetModuleInfo(module);
			std::size_t base = (std::size_t)module_info.lpBaseOfDll;

			std::span<IMAGE_SECTION_HEADER> section_headers = GetImageSectionHeaders(module);
			for (auto& section : section_headers) {
				if (strcmp((const char*)section.Name, ".text") == 0) {
					std::size_t section_base = base + (std::size_t)section.VirtualAddress;
					return (void*)section_base;
				}
			}
		}

		return nullptr;
	}
}
