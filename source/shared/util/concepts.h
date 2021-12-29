#pragma once

#include <type_traits>

namespace algo
{
template<class T, class U>
concept is_comparable = requires(T&& lhs, T&& rhs)
{
    lhs == rhs;
    rhs == lhs;
};

template<class T>
concept std_range = requires(T&& cont)
{
    std::begin(cont);
    std::end(cont);
};
template<class T>
concept not_std_range = !requires(T && cont)
{
    std::begin(cont);
    std::end(cont);
};
template<class T>
concept adl_range = requires(T&& cont)
{
    begin(cont);
    end(cont);
};
template<class T>
concept not_adl_range = !requires(T && cont)
{
    begin(cont);
    end(cont);
};
template<class T>
concept member_range = requires(T&& cont)
{
    cont.begin();
    cont.end();
};
template<class ContainerT>
requires member_range<ContainerT> && not_adl_range<ContainerT> && not_std_range<ContainerT>
constexpr auto get_begin(ContainerT&& cont)
{
    return cont.begin();
}
template<class ContainerT>
requires member_range<ContainerT> && not_adl_range<ContainerT> && not_std_range<ContainerT>
constexpr auto get_end(ContainerT&& cont)
{
    return cont.end();
}
template<class ContainerT>
requires adl_range<ContainerT> && not_std_range<ContainerT>
constexpr auto get_begin(ContainerT&& cont)
{
    return begin(cont);
}
template<class ContainerT>
requires adl_range<ContainerT> && not_std_range<ContainerT>
constexpr auto get_end(ContainerT&& cont)
{
    return end(cont);
}
template<class ContainerT>
requires std_range<ContainerT>
constexpr auto get_begin(ContainerT&& cont)
{
    return std::begin(cont);
}
template<class ContainerT>
requires std_range<ContainerT>
constexpr auto get_end(ContainerT&& cont)
{
    return std::end(cont);
}

template<class T>
concept range = requires(T&& cont)
{
    get_begin(cont);
    get_end(cont);
};

template<class T>
requires range<T>
using range_element_t = decltype(*get_begin(std::declval<T>()));
template<class T>
requires range<T>
using range_value_t = std::remove_reference_t<decltype(*get_begin(std::declval<T>()))>;

template<class T, class U>
requires range<T>
inline constexpr bool range_contains_v = std::is_same_v<std::decay_t<range_element_t<T>>, std::decay_t<U>>;
} // namespace algo
