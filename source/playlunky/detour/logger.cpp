#include "logger.h"
#include "log.h"

#include "detour_entry.h"
#include "detour_helper.h"
#include "sigfun.h"

#include <fmt/format.h>
#include <cstdio>
#include <memory>
#include <fstream>

struct DetourDoLog {
	inline static SigScan::Function<void(__stdcall*)(std::ofstream*, const char*, void*, LogLevel)> Trampoline{
		.Signature = "\x48\x89\x5c\x24\x10\x48\x89\x6c\x24\x18\x48\x89\x74\x24\x20\x57\x48\x83\xec\x40\x41\x0f\xb6\xe9"_sig
	};
	static void Detour(std::ofstream* stream, const char* message, void* param_3, LogLevel log_level) {
		fmt::print("{}\n", message);
		std::fflush(stdout);

		if (stream != nullptr) {
			Trampoline(stream, message, param_3, log_level);
		}
	}

	static void Log(const char* message, LogLevel log_level) {
		const std::string adjusted_message = fmt::format("Playlunky :: {}", message);
		DetourDoLog::Detour(s_Stream, adjusted_message.c_str(), nullptr, log_level);
	}

	inline static std::ofstream* s_Stream{ nullptr };
};

struct DetourConstructLog {
	inline static SigScan::Function<std::ofstream* (__stdcall*)(void*)> Trampoline{
		.Signature = "\x48\x89\x5c\x24\x20\x48\x89\x4c\x24\x08\x57\x48\x83\xec\x40\x48\x8b\xf9\xc7\x44\x24\x20\x00\x00\x00\x00"_sig
	};
	static std::ofstream* Detour(void* memory) {
		DetourDoLog::s_Stream = Trampoline(memory);
		return DetourDoLog::s_Stream;
	}
};

std::vector<DetourEntry> GetLogDetours() {
	return {
		DetourHelper<DetourDoLog>::GetDetourEntry("DoLog"),
		DetourHelper<DetourConstructLog>::GetDetourEntry("ConstructLog")
	};
}

void Log(const char* message, LogLevel log_level) {
	DetourDoLog::Log(message, log_level);
}
