#include "debug.h"

#include "detour_entry.h"
#include "detour_helper.h"
#include "sigscan.h"
#include "log.h"
#include "util.h"

#include <cstdint>

struct DetourReadEncrypedFile
{
	inline static SigScan::Function<void*(__stdcall*)(const char*, void* (*)(size_t size))> Trampoline{
		.Signature = "\x48\x8b\xc4\x48\x89\x58\x18\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8d\xa8\x38\xff\xff\xff"
	};
	static void* Detour(const char* file_path, void* (*alloc_fun)(size_t size))
	{
		FILE* file{ nullptr };
		auto error = fopen_s(&file, file_path, "rb");
		if (error == 0 && file != nullptr) {
			auto close_file = OnScopeExit{ [file]() { fclose(file); } };

			fseek(file, 0, SEEK_END);
			const auto size = ftell(file);
			fseek(file, 0, SEEK_SET);

			struct AssetInfo {
				void* Data;
				int FrameCount_0;
				int AssetSize;
				int AllocationSize;
				int FrameCount_1;
			};
			const auto allocation_size = size + static_cast<decltype(size)>(sizeof(AssetInfo));
			if (void* buf = alloc_fun(allocation_size)) {

				void* data = static_cast<void*>(reinterpret_cast<char*>(buf) + 24);
				const auto size_read = fread(data, 1, size, file);
				if (size_read != size) {
					LogInfo("Could not load asset {}, this will either crash or cause glitches...", file_path);
				}

				AssetInfo& asset_info = *static_cast<AssetInfo*>(buf);
				asset_info.Data = data;
				asset_info.FrameCount_0 = 0;
				asset_info.AssetSize = size;
				asset_info.AllocationSize = allocation_size;
				asset_info.FrameCount_1 = 0;

				return buf;
			}
		}
		return Trampoline(file_path, alloc_fun);
	}
};

std::vector<DetourEntry> GetFileIODetours() {
	return { DetourHelper<DetourReadEncrypedFile>::GetDetourEntry("ReadEncrypedFile") };
}
