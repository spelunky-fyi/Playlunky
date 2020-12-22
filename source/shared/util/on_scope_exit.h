#pragma once

#include <type_traits>
#include <windows.h>
#include <stdexcept>

template<class T>
bool GetProcAddress(HMODULE module_handle, LPCSTR proc_name, T** out_proc) {
	auto* found_proc = reinterpret_cast<T*>(GetProcAddress(module_handle, proc_name));
	if (found_proc != nullptr)
	{
		*out_proc = found_proc;
	}
	return found_proc != nullptr;
}

PSTR GetProcName(HMODULE module, const FARPROC proc_address);

template<class FunT>
requires std::is_invocable_r_v<void, FunT>
struct OnScopeExit {
	OnScopeExit(FunT&& fun) : Fun{ std::forward<FunT>(fun) } {}
	~OnScopeExit() { Fun(); }
	FunT Fun;
};
