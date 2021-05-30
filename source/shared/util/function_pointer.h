#pragma once

#include <type_traits>

namespace detail
{
template<class FunT, class FunctorT, class TagT>
struct FunctorFunctionPointer;
template<class TagT, class FunctorT, class Ret, class... Args>
struct FunctorFunctionPointer<Ret(Args...), FunctorT, TagT>
{
    static void Set(FunctorT&& functor)
    {
        s_Functor.emplace(std::forward<FunctorT>(functor));
    }
    static Ret Call(Args... args)
    {
        return s_Functor.value()(args...);
    }

    inline static std::optional<std::decay_t<FunctorT>> s_Functor;
};
template<class MemberFunT, class TagT>
struct MemberFunctionPointer;
template<class T, class Ret, class... Args, class TagT>
struct MemberFunctionPointer<Ret (T::*)(Args...), TagT>
{
    static void Set(Ret (T::*function)(Args...), T* object)
    {
        s_MemberFunction = function;
        s_Object = object;
    }
    static Ret Call(Args... args)
    {
        return (s_Object->*s_MemberFunction)(args...);
    }

    inline static Ret (T::*s_MemberFunction)(Args...){ nullptr };
    inline static T* s_Object{ nullptr };
};

template<class FunT, class OtherFunT>
struct is_invocable_as : std::false_type
{
};
template<class Ret, class... Args>
struct is_invocable_as<Ret(Args...), Ret(Args...)> : std::true_type
{
};
template<class Ret, class... Args, class T>
struct is_invocable_as<Ret(Args...), Ret (T::*)(Args...)> : std::true_type
{
};
template<class Ret, class... Args, class OtherFunT>
struct is_invocable_as<Ret(Args...), OtherFunT> : std::is_invocable_r<Ret, OtherFunT, Args...>
{
};
template<class FunT, class OtherFunT>
inline constexpr bool is_invocable_as_v = is_invocable_as<FunT, OtherFunT>::value;
}; // namespace detail

template<class FunT, class>
requires detail::is_invocable_as_v<FunT, FunT>
    FunT* FunctionPointer(FunT* function)
{
    return function;
};
template<class FunT, class TagT, class MemberFunT, class T>
requires detail::is_invocable_as_v<FunT, MemberFunT>
    FunT* FunctionPointer(MemberFunT function, T* val)
{
    detail::MemberFunctionPointer<MemberFunT, TagT>::Set(function, val);
    return &detail::MemberFunctionPointer<MemberFunT, TagT>::Call;
};
template<class FunT, class TagT, class FunctorT>
requires detail::is_invocable_as_v<FunT, FunctorT>
    FunT* FunctionPointer(FunctorT&& functor)
{
    detail::FunctorFunctionPointer<FunT, FunctorT, TagT>::Set(std::forward<FunctorT>(functor));
    return &detail::FunctorFunctionPointer<FunT, FunctorT, TagT>::Call;
};

// Use as:
/*
// Works for this one instantation until called again with the exact same lambda type
// Ideally use with lambdas that capture global data or singleton-like classes
FunctionPointer<bool(int, int)>([](int a, int b){ return a == b; });

// Works for this one instantion of MyClass until called again
// Ideally use for singelton-like classes
FunctionPointer<bool(int, int)>(&MyClass::Compare, this);

// Just passes through the function pointer
FunctionPointer<bool(int, int)>(&Compare);
*/
