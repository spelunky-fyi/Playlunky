
#pragma once

#include "sigfun.h"
#include "detour_entry.h"

#include <type_traits>

template<class DetourT>
struct DetourHelper {
	static DetourEntry GetDetourEntry(const char* function_name)
	{
		auto trampoline = DetourT::Trampoline;
		auto detour = &DetourT::Detour;
		using void_ptr = void*;

		if constexpr (std::is_same_v<decltype(trampoline), SigScan::Function<decltype(detour)>>) {
			auto trampoline_func = DetourT::Trampoline.Func;
			static_assert(std::is_same_v<decltype(trampoline_func), decltype(detour)>);

			return {
				.Trampoline = &(void_ptr&)DetourT::Trampoline.Func,
				.Detour = &DetourT::Detour,
				.Signature = &DetourT::Trampoline.Signature,
				.ProcName = DetourT::Trampoline.ProcName,
				.Module = DetourT::Trampoline.Module,
				.FunctionName = function_name
			};
		}
		else {
			static_assert(std::is_same_v<decltype(trampoline), decltype(detour)>);
			return {
				.Trampoline = &(void_ptr&)DetourT::Trampoline,
				.Detour = &DetourT::Detour,
				.Signature = nullptr,
				.ProcName = nullptr,
				.Module = nullptr,
				.FunctionName = function_name
			};
		}
	}
};
