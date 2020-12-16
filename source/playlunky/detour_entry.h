#pragma once

struct DetourEntry {
	void** Trampoline;
	void* Detour;
	const char** Signature;
};
