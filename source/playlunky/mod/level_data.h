#pragma once

#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

struct LevelSetting
{
    std::string Name;
    std::uint32_t Value;
};

struct TileCode
{
    std::uint8_t ShortCode;
    std::string TileOne;
    std::string TileTwo;
    std::uint32_t Chance;
};

struct LevelRoom
{
    std::string Name;
    std::uint32_t Width;
    std::uint32_t Height;
    std::vector<std::string> Flags;
    std::vector<std::uint8_t> FrontData;
    std::vector<std::uint8_t> BackData;

    template<auto LayerMember, class SelfT>
    auto Layer(this SelfT& self)
    {
        struct LayerProxy
        {
            SelfT& Self;
            operator SelfT&()
            {
                return Self;
            }

            auto operator[](this SelfT& self, std::size_t x)
            {
                struct IndexProxy
                {
                    SelfT& Self;
                    std::size_t X;

                    auto& operator[](std::size_t y)
                    {
                        return (Self.*LayerMember)[X + y * Self.Width];
                    }
                };

                return IndexProxy{ self, self.Flipped() ? self.Width - x - 1 : x };
            }
        };

        return LayerProxy{ self };
    }

    template<class SelfT>
    auto FrontLayer(this SelfT& self)
    {
        return self.Layer<&LevelRoom::FrontData>();
    }
    template<class SelfT>
    auto BackLayer(this SelfT& self)
    {
        return self.Layer<&LevelRoom::BackData>();
    }

    bool Flipped() const
    {
        using namespace std::string_view_literals;
        return std::find(Flags.begin(), Flags.end(), "onlyflip"sv) != Flags.end();
    }
};

struct LevelChance
{
    std::string Name;
    std::vector<std::uint32_t> Chances;
};

struct LevelData
{
    std::string Name;
    std::uint32_t Width;
    std::uint32_t Height;
    std::vector<LevelSetting> Settings;
    std::vector<TileCode> TileCodes;
    std::vector<LevelRoom> Rooms;
    std::vector<LevelChance> Chances;
    std::vector<LevelChance> MonsterChances;
};