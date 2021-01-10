#pragma once

namespace SigScan
{
	void* FindPattern(const char* signature, void* from, void* to);

	void* FindPattern(const char* module_name, const char* signature, bool code_only);
	void* FindPattern(const char* signature, bool code_only);

	using ptrdiff_t = decltype((char*)nullptr - (char*)nullptr);
	ptrdiff_t GetOffset(const char* module_name, const void* address);
	ptrdiff_t GetOffset(const void* address);

	void* GetDataSection();
};