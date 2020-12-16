#include "log.h"

#include "detour_entry.h"
#include "detour_helper.h"
#include "sigfun.h"

#include <fmt/format.h>
#include <cstdio>
#include <memory>
#include <fstream>

struct DetourDoLog
{
	inline static SigScan::Function<void(__stdcall*)(std::ofstream*, const char*, void*, LogLevel)> Trampoline{
		.Signature = "\x48\x89\x5c\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xec\x40\x41\x0f\xb6\xf1\x48\x8b\xfa\x48\x8b\xd9"
	};
	static void Detour(std::ofstream* stream, const char* message, void* param_3, LogLevel log_level) {
		fmt::print("{}\n", message);
		std::fflush(stdout);

		if (stream != nullptr) {
			Trampoline(stream, message, param_3, log_level);
		}
	}

	static void Log(const char* message, LogLevel log_level) {
		DetourDoLog::Detour(s_Stream, message, nullptr, log_level);
	}

	inline static std::ofstream* s_Stream{ nullptr };
};

std::vector<DetourEntry> GetLogDetours() {
	return { DetourHelper<DetourDoLog>::GetDetourEntry("DoLog") };
}

void Log(const char* message, LogLevel log_level) {
	DetourDoLog::Log(message, log_level);
}
