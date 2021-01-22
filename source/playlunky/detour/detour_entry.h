#pragma once

#include <string_view>

struct DetourEntry {
	void** Trampoline;
	void* Detour;
	std::string_view* Signature;
	const char* FunctionName;
};
