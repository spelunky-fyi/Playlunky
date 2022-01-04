#pragma once

#include <vector>

struct ColorRGB8
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;

    bool operator==(const ColorRGB8&) const = default;
};

struct ColorRGBA8
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;

    bool operator==(const ColorRGBA8&) const = default;
};

