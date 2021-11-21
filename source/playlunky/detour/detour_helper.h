
#pragma once

#include "detour_entry.h"
#include "sigfun.h"

#include <type_traits>

template<class FunT>
struct c_style_function;
template<typename T, typename R, typename... Args>
struct c_style_function<R (T::*)(Args...)>
{
    using type = R (*)(T*, Args...);
};
template<typename T, typename R, typename... Args>
struct c_style_function<R (T::*)(Args...) const>
{
    using type = R (*)(const T*, Args...);
};
template<class FunT>
using c_style_function_t = typename c_style_function<FunT>::type;

template<class DetourT>
struct DetourHelper
{
    static DetourEntry GetDetourEntry(const char* function_name)
    {
        auto trampoline = DetourT::Trampoline;
        auto detour = &DetourT::Detour;
        using void_ptr = void*;

        if constexpr (std::is_same_v<decltype(trampoline), SigScan::Function<decltype(detour)>>)
        {
            auto trampoline_func = DetourT::Trampoline.Func;
            static_assert(std::is_same_v<decltype(trampoline_func), decltype(detour)>);

            return {
                .Trampoline = &(void_ptr&)DetourT::Trampoline.Func,
                .Detour = &DetourT::Detour,
                .Signature = &DetourT::Trampoline.Signature,
                .FindFunctionStart = DetourT::Trampoline.FindFunctionStart,
                .ProcName = DetourT::Trampoline.ProcName,
                .Module = DetourT::Trampoline.Module,
                .FunctionName = function_name
            };
        }
        else if constexpr (std::is_member_function_pointer_v<decltype(trampoline)>)
        {
            static_assert(std::is_same_v<c_style_function_t<decltype(trampoline)>, decltype(detour)>);
            return {
                .Trampoline = &(void_ptr&)DetourT::Trampoline,
                .Detour = &DetourT::Detour,
                .Signature = nullptr,
                .ProcName = nullptr,
                .Module = nullptr,
                .FunctionName = function_name
            };
        }
        else
        {
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
