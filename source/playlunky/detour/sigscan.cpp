#include "sigscan.h"

#include <Windows.h>
#include <Psapi.h>
#include <winnt.h>

#include <cstdint>
#include <span>

namespace SigScan {
	void* FindPattern(std::string_view signature, void* from, void* to) {
		const std::size_t size = (char*)to - (char*)from;
		const std::size_t sig_length = signature.size();

		for (std::size_t j = 0; j < size - sig_length; j++) {
			bool found = true;
			for (std::size_t k = 0; k < sig_length && found; k++) {
				found = signature[k] == '*' || signature[k] == *((char*)from + j + k);
			}

			if (found) {
				return (char*)from + j;
			}
		}

		return nullptr;
	}

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

	std::size_t GetDataBundleSize(std::size_t bundle_start) {
		static const std::size_t bundle_size = [bundle_start]() {
			std::size_t bundle_size_compute{ 0 };
			while (true) {
				std::size_t asset_size = ((const std::uint32_t*)(bundle_start + bundle_size_compute))[0];
				std::size_t name_size = ((const std::uint32_t*)(bundle_start + bundle_size_compute))[1];
				if (asset_size == 0 && name_size == 0) {
					break;
				}
				bundle_size_compute += asset_size + name_size + 2 * sizeof(std::uint32_t);
			};
			return bundle_size_compute;
		}();
		return bundle_size;
	}

	void* FindPattern(const char* module_name, std::string_view signature, bool code_only) {
		if (module_name == nullptr || signature.empty())
			return nullptr;

		if (HMODULE module = GetModuleHandleA(module_name)) {
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
						section_start += GetDataBundleSize(section_base);
					}

					if (void* res = FindPattern(signature, (void*)(section_base + section_start), (void*)(section_base + size))) {
						return res;
					}
				}
			}
		}

		return nullptr;
	}

	std::ptrdiff_t GetOffset(const char* module_name, const void* address) {
		if (module_name == nullptr || address == nullptr)
			return 0;

		if (HMODULE module = GetModuleHandleA(module_name)) {
			return (char*)address - (char*)module;
		}

		return 0;
	}

	std::ptrdiff_t GetOffset(const void* address) {
		char module_name[MAX_PATH];
		GetModuleFileNameA(0, module_name, MAX_PATH);
		return GetOffset(module_name, address);
	}

	void* GetDataSection() {
		static void* const data_bundle_section = []() -> void* {
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
		}();
		return data_bundle_section;
	}
}
