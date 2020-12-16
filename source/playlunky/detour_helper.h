
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

			return { &(void_ptr&)DetourT::Trampoline.Func, &DetourT::Detour, &DetourT::Trampoline.Signature, function_name };
		}
		else {
			static_assert(std::is_same_v<decltype(trampoline), decltype(detour)>);
			return { Detour(&(void_ptr&)DetourT::Trampoline, &DetourT::Detour), nullptr, function_name };
		}
	}
};
