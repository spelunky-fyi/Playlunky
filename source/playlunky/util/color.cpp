#include "util/color.h"

#include <array>
#include <random>

std::mt19937& GetRandomColorState()
{
    static std::mt19937 state = []()
    {
        // Magic number 624: The number of unsigned ints the MT uses as state
        std::vector<unsigned int> random_data(624);
        std::random_device source;
        std::generate(std::begin(random_data), std::end(random_data), [&]()
                      { return source(); });
        std::seed_seq seeds(std::begin(random_data), std::end(random_data));
        std::mt19937 seeded_state{ seeds };
        return seeded_state;
    }();
    return state;
}

ColorRGB8 GenerateRandomColor()
{
    static auto& state{ GetRandomColorState() };
    static std::uniform_int_distribution dist{ 0, 255 };

    return ColorRGB8{
        .r{ static_cast<std::uint8_t>(dist(state)) },
        .g{ static_cast<std::uint8_t>(dist(state)) },
        .b{ static_cast<std::uint8_t>(dist(state)) },
    };
}

std::vector<ColorRGB8> GenerateDistinctRandomColors(std::size_t n, bool apply_variance)
{
    static constexpr std::array c_DistinctColors{
        ColorRGB8{.r{ 0x00 }, .g{ 0x00 }, .b{ 0x00 } },
        ColorRGB8{.r{ 0x00 }, .g{ 0xFF }, .b{ 0x00 } },
        ColorRGB8{.r{ 0x00 }, .g{ 0x00 }, .b{ 0xFF } },
        ColorRGB8{.r{ 0xFF }, .g{ 0x00 }, .b{ 0x00 } },
        ColorRGB8{.r{ 0x01 }, .g{ 0xFF }, .b{ 0xFE } },
        ColorRGB8{.r{ 0xFF }, .g{ 0xA6 }, .b{ 0xFE } },
        ColorRGB8{.r{ 0xFF }, .g{ 0xDB }, .b{ 0x66 } },
        ColorRGB8{.r{ 0x00 }, .g{ 0x64 }, .b{ 0x01 } },
        ColorRGB8{.r{ 0x01 }, .g{ 0x00 }, .b{ 0x67 } },
        ColorRGB8{.r{ 0x95 }, .g{ 0x00 }, .b{ 0x3A } },
        ColorRGB8{.r{ 0x00 }, .g{ 0x7D }, .b{ 0xB5 } },
        ColorRGB8{.r{ 0xFF }, .g{ 0x00 }, .b{ 0xF6 } },
        ColorRGB8{.r{ 0xFF }, .g{ 0xEE }, .b{ 0xE8 } },
        ColorRGB8{.r{ 0x77 }, .g{ 0x4D }, .b{ 0x00 } },
        ColorRGB8{.r{ 0x90 }, .g{ 0xFB }, .b{ 0x92 } },
        ColorRGB8{.r{ 0x00 }, .g{ 0x76 }, .b{ 0xFF } },
        ColorRGB8{.r{ 0xD5 }, .g{ 0xFF }, .b{ 0x00 } },
        ColorRGB8{.r{ 0xFF }, .g{ 0x93 }, .b{ 0x7E } },
        ColorRGB8{.r{ 0x6A }, .g{ 0x82 }, .b{ 0x6C } },
        ColorRGB8{.r{ 0xFF }, .g{ 0x02 }, .b{ 0x9D } },
        ColorRGB8{.r{ 0xFE }, .g{ 0x89 }, .b{ 0x00 } },
        ColorRGB8{.r{ 0x7A }, .g{ 0x47 }, .b{ 0x82 } },
        ColorRGB8{.r{ 0x7E }, .g{ 0x2D }, .b{ 0xD2 } },
        ColorRGB8{.r{ 0x85 }, .g{ 0xA9 }, .b{ 0x00 } },
        ColorRGB8{.r{ 0xFF }, .g{ 0x00 }, .b{ 0x56 } },
        ColorRGB8{.r{ 0xA4 }, .g{ 0x24 }, .b{ 0x00 } },
        ColorRGB8{.r{ 0x00 }, .g{ 0xAE }, .b{ 0x7E } },
        ColorRGB8{.r{ 0x68 }, .g{ 0x3D }, .b{ 0x3B } },
        ColorRGB8{.r{ 0xBD }, .g{ 0xC6 }, .b{ 0xFF } },
        ColorRGB8{.r{ 0x26 }, .g{ 0x34 }, .b{ 0x00 } },
        ColorRGB8{.r{ 0xBD }, .g{ 0xD3 }, .b{ 0x93 } },
        ColorRGB8{.r{ 0x00 }, .g{ 0xB9 }, .b{ 0x17 } },
        ColorRGB8{.r{ 0x9E }, .g{ 0x00 }, .b{ 0x8E } },
        ColorRGB8{.r{ 0x00 }, .g{ 0x15 }, .b{ 0x44 } },
        ColorRGB8{.r{ 0xC2 }, .g{ 0x8C }, .b{ 0x9F } },
        ColorRGB8{.r{ 0xFF }, .g{ 0x74 }, .b{ 0xA3 } },
        ColorRGB8{.r{ 0x01 }, .g{ 0xD0 }, .b{ 0xFF } },
        ColorRGB8{.r{ 0x00 }, .g{ 0x47 }, .b{ 0x54 } },
        ColorRGB8{.r{ 0xE5 }, .g{ 0x6F }, .b{ 0xFE } },
        ColorRGB8{.r{ 0x78 }, .g{ 0x82 }, .b{ 0x31 } },
        ColorRGB8{.r{ 0x0E }, .g{ 0x4C }, .b{ 0xA1 } },
        ColorRGB8{.r{ 0x91 }, .g{ 0xD0 }, .b{ 0xCB } },
        ColorRGB8{.r{ 0xBE }, .g{ 0x99 }, .b{ 0x70 } },
        ColorRGB8{.r{ 0x96 }, .g{ 0x8A }, .b{ 0xE8 } },
        ColorRGB8{.r{ 0xBB }, .g{ 0x88 }, .b{ 0x00 } },
        ColorRGB8{.r{ 0x43 }, .g{ 0x00 }, .b{ 0x2C } },
        ColorRGB8{.r{ 0xDE }, .g{ 0xFF }, .b{ 0x74 } },
        ColorRGB8{.r{ 0x00 }, .g{ 0xFF }, .b{ 0xC6 } },
        ColorRGB8{.r{ 0xFF }, .g{ 0xE5 }, .b{ 0x02 } },
        ColorRGB8{.r{ 0x62 }, .g{ 0x0E }, .b{ 0x00 } },
        ColorRGB8{.r{ 0x00 }, .g{ 0x8F }, .b{ 0x9C } },
        ColorRGB8{.r{ 0x98 }, .g{ 0xFF }, .b{ 0x52 } },
        ColorRGB8{.r{ 0x75 }, .g{ 0x44 }, .b{ 0xB1 } },
        ColorRGB8{.r{ 0xB5 }, .g{ 0x00 }, .b{ 0xFF } },
        ColorRGB8{.r{ 0x00 }, .g{ 0xFF }, .b{ 0x78 } },
        ColorRGB8{.r{ 0xFF }, .g{ 0x6E }, .b{ 0x41 } },
        ColorRGB8{.r{ 0x00 }, .g{ 0x5F }, .b{ 0x39 } },
        ColorRGB8{.r{ 0x6B }, .g{ 0x68 }, .b{ 0x82 } },
        ColorRGB8{.r{ 0x5F }, .g{ 0xAD }, .b{ 0x4E } },
        ColorRGB8{.r{ 0xA7 }, .g{ 0x57 }, .b{ 0x40 } },
        ColorRGB8{.r{ 0xA5 }, .g{ 0xFF }, .b{ 0xD2 } },
        ColorRGB8{.r{ 0xFF }, .g{ 0xB1 }, .b{ 0x67 } },
        ColorRGB8{.r{ 0x00 }, .g{ 0x9B }, .b{ 0xFF } },
        ColorRGB8{.r{ 0xE8 }, .g{ 0x5E }, .b{ 0xBE } },
    };

    std::vector<ColorRGB8> colors{ c_DistinctColors.begin(), c_DistinctColors.end() };

    while (n > colors.size())
    {
        colors.insert(colors.end(), c_DistinctColors.begin(), c_DistinctColors.end());
    }

    static auto& state{ GetRandomColorState() };
    while (colors.size() > n)
    {
        colors.erase(colors.begin() + state() % colors.size());
    }
    std::shuffle(colors.begin(), colors.end(), state);

    if (apply_variance)
    {
        std::uniform_real_distribution<float> dist{ 0.95f, 1.05f };
        for (ColorRGB8& color : colors)
        {
            color.r = static_cast<std::uint8_t>(std::clamp(color.r * dist(state), 0.0f, 255.0f));
            color.g = static_cast<std::uint8_t>(std::clamp(color.g * dist(state), 0.0f, 255.0f));
            color.b = static_cast<std::uint8_t>(std::clamp(color.b * dist(state), 0.0f, 255.0f));
        }
    }

    return colors;
}
