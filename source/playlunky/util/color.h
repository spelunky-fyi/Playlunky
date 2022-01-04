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

// Truly unique colors
ColorRGB8 GenerateRandomColor();
// Get a set of unique and distinct colors, only works nicely for small n
std::vector<ColorRGB8> GenerateDistinctRandomColors(std::size_t n, bool apply_variance = true);
