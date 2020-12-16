#include "detour.h"

#include "detour_entry.h"
#include "sigscan.h"
#include "debug.h"
#include "file_io.h"
#include "log.h"

#include <Windows.h>

#include <detours/detours.h>
#include <fmt/core.h>
#include <fmt/format.h>

#include <span>

#define DLL_NAME "playlunky" DETOURS_STRINGIFY(DETOURS_BITS) ".dll"

struct ByteStr { const char* Str; };
template<>
struct fmt::formatter<ByteStr> {
	constexpr auto parse(format_parse_context& ctx) {
		return ctx.begin();
	}

	template <typename FormatContext>
	auto format(const ByteStr& byte_str, FormatContext& ctx) {
		std::span<uint8_t> byte_span{ (uint8_t*)byte_str.Str, strlen(byte_str.Str) };
		auto out = ctx.out();
		for (uint8_t c : byte_span) {
			out = format_to(out, "\\0x{:x}", c);
		}
		return out;
	}
};

std::vector<DetourEntry> CollectDetourEntries() {
	auto append = [](std::vector<DetourEntry>& dst, std::vector<DetourEntry> src) {
		std::move(src.begin(), src.end(), std::back_inserter(dst));
	};

	std::vector<DetourEntry> detour_entries;
	append(detour_entries, GetLogDetours());
	append(detour_entries, GetDebugDetours());
	append(detour_entries, GetFileIODetours());
	return detour_entries;
}

void Attach() {
	fmt::print(DLL_NAME ": Attaching...\n");

	std::vector<DetourEntry> detour_entries = CollectDetourEntries();

	DetourRestoreAfterWith();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	for (auto [trampoline, detour, signature] : detour_entries) {
		if (signature != nullptr) {
			*trampoline = SigScan::FindPattern(*signature);
			if (*trampoline != nullptr)
			{
				fmt::print("Found function with signature {}\n\t\tat address {}\n", ByteStr{ .Str = *signature }, *trampoline);
			}
		}

		if (*trampoline == nullptr) {
			fmt::print("Can not detour function {}, no valid source function specified\n", detour);
		}
		else {
			DetourAttach(trampoline, detour);
		}
	}

	const LONG error = DetourTransactionCommit();
	if (error == NO_ERROR) {
		fmt::print(DLL_NAME ": Succees...\n");
	}
	else {
		fmt::print(DLL_NAME ": Error: {}\n", error);
	}
	std::fflush(stdout);
}

void Detach() {
	fmt::print(DLL_NAME ": Dettaching...\n");

	std::vector<DetourEntry> detour_entries = CollectDetourEntries();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	for (auto [trampoline, detour, signature] : detour_entries) {
		DetourDetach(trampoline, detour);
	}

	const LONG error = DetourTransactionCommit();
	if (error == NO_ERROR) {
		fmt::print(DLL_NAME ": Succees...\n");
	}
	else {
		fmt::print(DLL_NAME ": Error: {}\n", error);
	}
	std::fflush(stdout);
}
