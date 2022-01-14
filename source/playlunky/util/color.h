#pragma once

#include <compare>
#include <vector>

struct ColorRGB8
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;

    std::weak_ordering operator<=>(const ColorRGB8&) const = default;
};
using ColorHSL8 = ColorRGB8;

struct ColorRGBA8
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;

    std::weak_ordering operator<=>(const ColorRGBA8&) const = default;
};

namespace ColorLiterals
{
constexpr ColorRGB8 operator"" _rgb(unsigned long long hex)
{
    return ColorRGB8{
        .r{ static_cast<std::uint8_t>((hex & 0xff0000) >> 16) },
        .g{ static_cast<std::uint8_t>((hex & 0x00ff00) >> 8) },
        .b{ static_cast<std::uint8_t>(hex & 0x0000ff) },
    };
}
constexpr ColorRGBA8 operator"" _rgba(unsigned long long hex)
{
    return ColorRGBA8{
        .r{ static_cast<std::uint8_t>((hex & 0xff000000) >> 24) },
        .g{ static_cast<std::uint8_t>((hex & 0x00ff0000) >> 16) },
        .b{ static_cast<std::uint8_t>((hex & 0x0000ff00) >> 8) },
        .a{ static_cast<std::uint8_t>(hex & 0x000000ff) },
    };
}
} // namespace ColorLiterals

float GetLuminance(float r, float g, float b);
std::tuple<float, float, float> SetLuminance(float r, float g, float b, float l);

ColorHSL8 ConvertRGB2HSL(ColorRGB8 color_rgb);
ColorRGB8 ConvertHSL2RGB(ColorHSL8 color_hsl);

// Truly unique colors
ColorRGB8 GenerateRandomColor();
// Generates n truly random colors
std::vector<ColorRGB8> GenerateRandomColors(std::size_t n);
// Get a set of unique and distinct colors, only works nicely for small n
std::vector<ColorRGB8> GenerateDistinctRandomColors(std::size_t n, bool apply_variance = true);
