#include "get_proc.h"

inline PSTR GetProcName(HMODULE module, const FARPROC proc_address)
{
	PIMAGE_DOS_HEADER dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
	if (!dos_header || dos_header->e_magic != IMAGE_DOS_SIGNATURE)
		throw std::runtime_error("Process::customGetProcAddress Error : DOS PE header is invalid.");

	PIMAGE_NT_HEADERS nt_header = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<PCHAR>(module) + dos_header->e_lfanew);
	if (nt_header->Signature != IMAGE_NT_SIGNATURE)
		throw std::runtime_error("Process::customGetProcAddress Error : NT PE header is invalid.");

	PVOID export_dir_temp = reinterpret_cast<PBYTE>(module) + nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	PIMAGE_EXPORT_DIRECTORY pExportDir = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(export_dir_temp);
	if (pExportDir->AddressOfNames == NULL)
		throw std::runtime_error("Process::customGetProcAddress Error : Symbol names missing entirely.");

	PDWORD names_rvas = reinterpret_cast<PDWORD>(reinterpret_cast<PBYTE>(module) + pExportDir->AddressOfNames);
	PWORD name_ordinals = reinterpret_cast<PWORD>(reinterpret_cast<PBYTE>(module) + pExportDir->AddressOfNameOrdinals);
	PDWORD function_addresses = reinterpret_cast<PDWORD>(reinterpret_cast<PBYTE>(module) + pExportDir->AddressOfFunctions);

	for (DWORD n = 0; n < pExportDir->NumberOfNames; n++)
	{
		WORD ordinal = name_ordinals[n];
		FARPROC current_address = reinterpret_cast<FARPROC>(reinterpret_cast<PBYTE>(module) + function_addresses[ordinal]);
		if (current_address == proc_address)
		{
			return reinterpret_cast<PSTR>(reinterpret_cast<PBYTE>(module) + names_rvas[n]);
		}
	}

	return nullptr;
}
