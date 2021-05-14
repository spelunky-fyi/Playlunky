#pragma once

#include <stdexcept>
#include <type_traits>
#include <windows.h>

template<class T>
bool GetProcAddress(HMODULE module_handle, LPCSTR proc_name, T** out_proc)
{
    auto* found_proc = reinterpret_cast<T*>(GetProcAddress(module_handle, proc_name));
    if (found_proc != nullptr)
    {
        *out_proc = found_proc;
    }
    return found_proc != nullptr;
}

PSTR GetProcName(HMODULE module, const FARPROC proc_address);
