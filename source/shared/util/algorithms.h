#pragma once

#include "concepts.h"
#include "tokenize.h"

#include <filesystem>
#include <functional>
#include <string_view>
#include <utility>
#include <vector>

namespace algo
{
template<class ContainerT, class FunT>
requires range<ContainerT> && std::is_invocable_r_v<bool, FunT, range_element_t<ContainerT>>
auto all_of(ContainerT&& container, FunT&& fun)
{
    const auto begin_it = get_begin(container);
    const auto end_it = get_end(container);
    return std::all_of(begin_it, end_it, std::forward<FunT>(fun));
}
template<class ContainerT, class FunT>
requires range<ContainerT> && std::is_invocable_r_v<bool, FunT, range_element_t<ContainerT>>
auto any_of(ContainerT&& container, FunT&& fun)
{
    const auto begin_it = get_begin(container);
    const auto end_it = get_end(container);
    return std::any_of(begin_it, end_it, std::forward<FunT>(fun));
}
template<class ContainerT, class FunT>
requires range<ContainerT> && std::is_invocable_r_v<bool, FunT, range_element_t<ContainerT>>
auto none_of(ContainerT&& container, FunT&& fun)
{
    const auto begin_it = get_begin(container);
    const auto end_it = get_end(container);
    return std::none_of(begin_it, end_it, std::forward<FunT>(fun));
}

template<class ContainerT, class FunT>
requires range<ContainerT> && std::is_invocable_r_v<bool, FunT, range_element_t<ContainerT>>
void erase_if(ContainerT&& container, FunT&& fun)
{
    const auto begin_it = get_begin(container);
    const auto end_it = get_end(container);
    container.erase(std::remove_if(begin_it, end_it, std::forward<FunT>(fun)), end_it);
}
template<class ContainerT, class ValueT>
requires range<ContainerT> && is_comparable<range_element_t<ContainerT>, ValueT>
void erase(ContainerT&& container, ValueT&& value)
{
    const auto begin_it = get_begin(container);
    const auto end_it = get_end(container);
    container.erase(std::remove(begin_it, end_it, std::forward<ValueT>(value)), end_it);
}
template<class ContainerT, class T, class U, class V>
requires range<ContainerT> && range_contains_v<ContainerT, T> && is_comparable<U, V>
void erase(ContainerT&& container, U T::*member, V&& val)
{
    erase_if(std::forward<ContainerT>(container), [member = std::mem_fn(member), val = std::forward<V>(val)](auto& element)
             { return member(element) == val; });
}

template<class ContainerT, class FunT>
requires range<ContainerT> && std::is_invocable_r_v<bool, FunT, range_element_t<ContainerT>>
auto find_if(ContainerT&& container, FunT&& fun) -> range_value_t<ContainerT>*
{
    const auto begin_it = get_begin(container);
    const auto end_it = get_end(container);
    auto found_it = std::find_if(begin_it, end_it, std::forward<FunT>(fun));
    if (found_it != end_it)
    {
        return &*found_it;
    }
    return nullptr;
}
template<class ContainerT, class ValueT>
requires range<ContainerT> && is_comparable<range_element_t<ContainerT>, ValueT>
auto find(ContainerT&& container, ValueT&& value) -> range_value_t<ContainerT>*
{
    const auto begin_it = get_begin(container);
    const auto end_it = get_end(container);
    auto found_it = std::find(begin_it, end_it, std::forward<ValueT>(value));
    if (found_it != end_it)
    {
        return &*found_it;
    }
    return nullptr;
}
template<class ContainerT, class T, class U, class V>
requires range<ContainerT> && range_contains_v<ContainerT, T> && is_comparable<U, V>
auto find(ContainerT&& container, U T::*member, V&& val) -> range_value_t<ContainerT>*
{
    return find_if(std::forward<ContainerT>(container), [member = std::mem_fn(member), val = std::forward<V>(val)](auto& element)
                   { return member(element) == val; });
}
template<class ContainerT, class T, class U, class V>
requires range<ContainerT> && range_contains_v<ContainerT, T> && is_comparable<U, V>
auto find(ContainerT&& container, U (T::*member)() const noexcept, V&& val) -> range_value_t<ContainerT>*
{
    return find_if(std::forward<ContainerT>(container), [member = std::mem_fn(member), val = std::forward<V>(val)](auto& element)
                   { return member(element) == val; });
}

template<class ContainerT, class FunT>
requires range<ContainerT> && std::is_invocable_r_v<bool, FunT, range_element_t<ContainerT>>
bool contains_if(ContainerT&& container, FunT&& fun)
{
    return find_if(std::forward<ContainerT>(container), std::forward<FunT>(fun)) != nullptr;
}
template<class ContainerT, class T, class U, class V>
requires range<ContainerT> && range_contains_v<ContainerT, T> && is_comparable<U, V>
bool contains(ContainerT&& container, U T::*member, V&& val)
{
    return contains_if(std::forward<ContainerT>(container), [member = std::mem_fn(member), val = std::forward<V>(val)](auto& element)
                       { return member(element) == val; });
}
template<class ContainerT, class T, class U, class V>
requires range<ContainerT> && range_contains_v<ContainerT, T> && is_comparable<U, V>
bool contains(ContainerT&& container, U (T::*member)() const, V&& val)
{
    return contains_if(std::forward<ContainerT>(container), [member = std::mem_fn(member), val = std::forward<V>(val)](auto& element)
                       { return member(element) == val; });
}
template<class ContainerT, class ValueT>
requires range<ContainerT> && is_comparable<range_element_t<ContainerT>, ValueT>
bool contains(ContainerT&& container, ValueT&& value)
{
    const auto begin_it = get_begin(container);
    const auto end_it = get_end(container);
    return std::find(begin_it, end_it, std::forward<ValueT>(value)) != end_it;
}

template<class ContainerT>
requires range<ContainerT>
void sort(ContainerT&& container)
{
    const auto begin_it = get_begin(container);
    const auto end_it = get_end(container);
    std::sort(begin_it, end_it);
}
template<class ContainerT, class FunT>
requires range<ContainerT> && std::is_invocable_r_v<bool, FunT, range_element_t<ContainerT>, range_element_t<ContainerT>>
void sort(ContainerT&& container, FunT&& fun)
{
    const auto begin_it = get_begin(container);
    const auto end_it = get_end(container);
    std::sort(begin_it, end_it, std::forward<FunT>(fun));
}
template<class ContainerT, class T, class U>
requires range<ContainerT> && range_contains_v<ContainerT, T>
void sort(ContainerT&& container, U T::*member)
{
    sort(std::forward<ContainerT>(container), [member = std::mem_fn(member)](auto& lhs, auto& rhs)
         { return member(lhs) < member(rhs); });
}
template<class ContainerT, class T, class U>
requires range<ContainerT> && range_contains_v<ContainerT, T>
void sort(ContainerT&& container, U (T::*member)() const)
{
    sort(std::forward<ContainerT>(container), [member = std::mem_fn(member)](auto& lhs, auto& rhs)
         { return member(lhs) < member(rhs); });
}

template<class ContainerT>
requires range<ContainerT>
bool is_sub_set(ContainerT&& sub_set, ContainerT&& container)
{
    return all_of(std::forward<ContainerT>(sub_set), [container = std::forward<ContainerT>(container)](const auto& val)
                  { return contains(container, val); });
}

template<class ContainerT, class ValueT>
requires range<ContainerT> && is_comparable<range_element_t<ContainerT>, ValueT>
auto count(ContainerT&& container, ValueT&& value)
{
    const auto begin_it = get_begin(container);
    const auto end_it = get_end(container);
    return static_cast<std::size_t>(std::count(begin_it, end_it, std::forward<ValueT>(value)));
}
template<class ContainerT, class FunT>
requires range<ContainerT> && std::is_invocable_r_v<bool, FunT, range_element_t<ContainerT>>
auto count_if(ContainerT&& container, FunT&& fun)
{
    const auto begin_it = get_begin(container);
    const auto end_it = get_end(container);
    return static_cast<std::size_t>(std::count_if(begin_it, end_it, std::forward<FunT>(fun)));
}

std::string path_string(const std::filesystem::path& path);
bool is_same_path(const std::filesystem::path& lhs, const std::filesystem::path& rhs);
bool is_sub_path(const std::filesystem::path& path, const std::filesystem::path& base);
bool is_end_of_path(const std::filesystem::path& path, const std::filesystem::path& base);
std::filesystem::path strip_end_of_path(const std::filesystem::path& path, const std::filesystem::path& base);

std::string trim(std::string str);
std::string trim(std::string str, char to_strip);

template<char Delimeter>
std::vector<std::string_view> split(std::string_view str)
{
    std::vector<std::string_view> sub_strings;
    for (auto&& sub_string : Tokenize<Delimeter>{ str })
    {
        sub_strings.push_back(sub_string);
    }
    return sub_strings;
}

std::string to_lower(std::string str);
std::string to_upper(std::string str);

bool case_insensitive_equal(std::string_view lhs, std::string_view rhs);
template<std::size_t N>
inline bool case_insensitive_equal(char (&lhs)[N], std::string_view rhs)
{
    return case_insensitive_equal(std::string_view{ lhs, N }, rhs);
}
template<std::size_t N>
inline bool case_insensitive_equal(std::string_view lhs, char (&rhs)[N])
{
    return case_insensitive_equal(lhs, std::string_view{ rhs, N });
}
template<std::size_t N, std::size_t M>
inline bool case_insensitive_equal(char (&lhs)[N], char (&rhs)[M])
{
    return case_insensitive_equal(std::string_view{ lhs, N }, std::string_view{ rhs, M });
}

// Intentionally copies args, require std::reference_wrapper if people want references
// https://github.com/lefticus/tools/blob/main/include/lefticus/tools/curry.hpp
constexpr decltype(auto) curry(auto f, auto... ps)
{
    if constexpr (requires { std::invoke(f, ps...); })
    {
        return std::invoke(f, ps...);
    }
    else
    {
        return [f, ps...](auto... qs) -> decltype(auto)
        { return curry(f, ps..., qs...); };
    }
}
// https://www.youtube.com/watch?v=s2Kqcn5e73c
constexpr decltype(auto) bind_front(auto f, auto... ps)
{
    return [f, ps...](auto... qs) -> decltype(auto)
    { return std::invoke(f, ps..., qs...); };
}
constexpr decltype(auto) bind_back(auto f, auto... ps)
{
    return [f, ps...](auto... qs) -> decltype(auto)
    { return std::invoke(f, qs..., ps...); };
}
} // namespace algo
