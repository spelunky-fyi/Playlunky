#pragma once

#include <span>

namespace span
{
template<class T>
std::span<T> bit_cast(std::span<std::uint8_t> data)
{
    if (data.size() % sizeof(T) == 0)
    {
        return { reinterpret_cast<T*>(data.data()), data.size() / sizeof(T) };
    }
    return {};
}
template<class T>
std::span<T> bit_cast_allow_size_mismatch(std::span<std::uint8_t> data)
{
    return std::span<T>{ reinterpret_cast<T*>(data.data()), data.size() / sizeof(T) };
}
} // namespace span
