#pragma once

#include <string_view>

namespace SigScan
{
	void* FindPattern(std::string_view signature, void* from, void* to);

	void* FindPattern(const char* module_name, std::string_view signature, bool code_only);
	void* FindPattern(std::string_view signature, bool code_only);

	using ptrdiff_t = decltype((char*)nullptr - (char*)nullptr);
	ptrdiff_t GetOffset(const char* module_name, const void* address);
	ptrdiff_t GetOffset(const void* address);

	void* GetDataSection();
};