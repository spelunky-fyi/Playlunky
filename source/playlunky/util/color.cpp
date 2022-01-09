#include "util/color.h"

#include <array>
#include <random>
#include <span>
#include <utility>

ColorHSL8 ConvertRGB2HSL(ColorRGB8 color_rgb)
{
    float r = static_cast<float>(color_rgb.r) / 255.0f;
    float g = static_cast<float>(color_rgb.g) / 255.0f;
    float b = static_cast<float>(color_rgb.b) / 255.0f;

    float h, s, l;
    {
        float K = 0.f;
        if (g < b)
        {
            std::swap(g, b);
            K = -1.f;
        }
        if (r < g)
        {
            std::swap(r, g);
            K = -2.f / 6.f - K;
        }

        const float chroma = r - (g < b ? g : b);
        h = std::fabs(K + (g - b) / (6.f * chroma + 1e-20f));
        s = chroma / (r + 1e-20f);
        l = r;
    }
    return ColorHSL8{
        static_cast<std::uint8_t>(h * 255.0f),
        static_cast<std::uint8_t>(s * 255.0f),
        static_cast<std::uint8_t>(l * 255.0f),
    };
}
ColorRGB8 ConvertHSL2RGB(ColorHSL8 color_hsl)
{
    float h = static_cast<float>(color_hsl.r) / 255.0f;
    float s = static_cast<float>(color_hsl.g) / 255.0f;
    float l = static_cast<float>(color_hsl.b) / 255.0f;

    float r, g, b;
    {
        if (s == 0.0f)
        {
            // gray
            r = g = b = l;
            return ColorHSL8{
                static_cast<std::uint8_t>(r * 255.0f),
                static_cast<std::uint8_t>(g * 255.0f),
                static_cast<std::uint8_t>(b * 255.0f),
            };
        }

        l = std::fmod(h, 1.0f) / (60.0f / 360.0f);
        int i = (int)h;
        float f = h - (float)i;
        float p = l * (1.0f - s);
        float q = l * (1.0f - s * f);
        float t = l * (1.0f - s * (1.0f - f));

        switch (i)
        {
        case 0:
            r = l;
            g = t;
            b = p;
            break;
        case 1:
            r = q;
            g = l;
            b = p;
            break;
        case 2:
            r = p;
            g = l;
            b = t;
            break;
        case 3:
            r = p;
            g = q;
            b = l;
            break;
        case 4:
            r = t;
            g = p;
            b = l;
            break;
        case 5:
        default:
            r = l;
            g = p;
            b = q;
            break;
        }
    }
    return ColorHSL8{
        static_cast<std::uint8_t>(r * 255.0f),
        static_cast<std::uint8_t>(g * 255.0f),
        static_cast<std::uint8_t>(b * 255.0f),
    };
}

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

std::vector<ColorRGB8> GenerateRandomColors(std::size_t n)
{
    std::vector<ColorRGB8> colors;
    for (size_t i = 0; i < n; i++)
    {
        colors.push_back(ColorRGB8{ static_cast<std::uint8_t>(rand() % 255), static_cast<std::uint8_t>(rand() % 255), static_cast<std::uint8_t>(rand() % 255) });
    }
    return colors;
}

std::vector<ColorRGB8> GenerateDistinctRandomColors(std::size_t n, bool apply_variance)
{
    using namespace ColorLiterals;

    static constexpr std::array c_DistinctColors{
        0x000000_rgb,
        0x00FF00_rgb,
        0x0000FF_rgb,
        0xFF0000_rgb,
        0x01FFFE_rgb,
        0xFFA6FE_rgb,
        0xFFDB66_rgb,
        0x006401_rgb,
        0x010067_rgb,
        0x95003A_rgb,
        0x007DB5_rgb,
        0xFF00F6_rgb,
        0xFFEEE8_rgb,
        0x774D00_rgb,
        0x90FB92_rgb,
        0x0076FF_rgb,
        0xD5FF00_rgb,
        0xFF937E_rgb,
        0x6A826C_rgb,
        0xFF029D_rgb,
        0xFE8900_rgb,
        0x7A4782_rgb,
        0x7E2DD2_rgb,
        0x85A900_rgb,
        0xFF0056_rgb,
        0xA42400_rgb,
        0x00AE7E_rgb,
        0x683D3B_rgb,
        0xBDC6FF_rgb,
        0x263400_rgb,
        0xBDD393_rgb,
        0x00B917_rgb,
        0x9E008E_rgb,
        0x001544_rgb,
        0xC28C9F_rgb,
        0xFF74A3_rgb,
        0x01D0FF_rgb,
        0x004754_rgb,
        0xE56FFE_rgb,
        0x788231_rgb,
        0x0E4CA1_rgb,
        0x91D0CB_rgb,
        0xBE9970_rgb,
        0x968AE8_rgb,
        0xBB8800_rgb,
        0x43002C_rgb,
        0xDEFF74_rgb,
        0x00FFC6_rgb,
        0xFFE502_rgb,
        0x620E00_rgb,
        0x008F9C_rgb,
        0x98FF52_rgb,
        0x7544B1_rgb,
        0xB500FF_rgb,
        0x00FF78_rgb,
        0xFF6E41_rgb,
        0x005F39_rgb,
        0x6B6882_rgb,
        0x5FAD4E_rgb,
        0xA75740_rgb,
        0xA5FFD2_rgb,
        0xFFB167_rgb,
        0x009BFF_rgb,
        0xE85EBE_rgb,
    };

    std::span<const ColorRGB8> color_table{ c_DistinctColors.begin(), c_DistinctColors.end() };

    static constexpr auto c_GapBetweenPride = 15u;
    static constexpr auto c_LengthOfPride = 2u;
    static auto s_NumCalls = c_LengthOfPride;
    if (++s_NumCalls % c_GapBetweenPride < c_LengthOfPride)
    {
        color_table = [](auto i) -> std::span<const ColorRGB8>
        {
            switch (i % 4)
            {
            default:
            case 0:
            {
                static constexpr std::array pride_colors = {
                    0xFF0018_rgb,
                    0xFFA52C_rgb,
                    0xFFFF41_rgb,
                    0x008018_rgb,
                    0x0000F9_rgb,
                    0x86007D_rgb,
                };
                return { pride_colors.begin(), pride_colors.end() };
            }
            case 1:
            {
                static constexpr std::array trans_colors = {
                    0x55CDFC_rgb,
                    0xF7A8B8_rgb,
                    0xFFFFFF_rgb,
                    0xF7A8B8_rgb,
                    0x55CDFC_rgb,
                };
                return { trans_colors.begin(), trans_colors.end() };
            }
            case 2:
            {
                static constexpr std::array enby_colors = {
                    0xFFF430_rgb,
                    0xFFFFFF_rgb,
                    0x9C59D1_rgb,
                    0x000000_rgb,
                };
                return { enby_colors.begin(), enby_colors.end() };
            }
            case 3:
            {
                static constexpr std::array bi_colors = {
                    0xD8097E_rgb,
                    0x8C579C_rgb,
                    0x24468E_rgb,
                };
                return { bi_colors.begin(), bi_colors.end() };
            }
            }
        }(s_NumCalls / c_GapBetweenPride - 1);
        apply_variance = false;
    }

    std::vector<ColorRGB8> colors{ color_table.begin(), color_table.end() };

    while (n > colors.size())
    {
        colors.insert(colors.end(), color_table.begin(), color_table.end());
    }

    if (apply_variance)
    {
        static auto& state{ GetRandomColorState() };
        while (colors.size() > n)
        {
            colors.erase(colors.begin() + state() % colors.size());
        }
        std::shuffle(colors.begin(), colors.end(), state);

        std::uniform_real_distribution<float> dist{ 0.95f, 1.05f };
        for (ColorRGB8& color : colors)
        {
            color.r = static_cast<std::uint8_t>(std::clamp(color.r * dist(state), 0.0f, 255.0f));
            color.g = static_cast<std::uint8_t>(std::clamp(color.g * dist(state), 0.0f, 255.0f));
            color.b = static_cast<std::uint8_t>(std::clamp(color.b * dist(state), 0.0f, 255.0f));
        }
    }
    else
    {
        colors.erase(colors.begin() + n, colors.end());
    }

    return colors;
}
