#pragma once

#include <filesystem>
#include <functional>
#include <vector>

#include "util/image.h"

struct SheetSize
{
    std::uint32_t Width;
    std::uint32_t Height;
};
struct Tile
{
    std::uint32_t Left;
    std::uint32_t Top;
    std::uint32_t Right;
    std::uint32_t Bottom;

    bool operator==(const Tile&) const = default;
};
struct TileMapping
{
    Tile SourceTile;
    Tile TargetTile;

    bool operator==(const TileMapping&) const = default;
};
struct SourceSheet
{
    std::filesystem::path Path;
    std::optional<std::filesystem::path> RootPath;
    std::int64_t Priority{ std::numeric_limits<std::int64_t>::max() };
    SheetSize Size;
    std::vector<TileMapping> TileMap;
    std::function<Image(Image, ImageSize)> Processing;
};
struct MultiSourceTile
{
    std::vector<std::filesystem::path> Paths;
    std::vector<SheetSize> Sizes;
    std::vector<TileMapping> TileMap;
    std::function<Image(std::vector<std::pair<Image, std::filesystem::path>>, ImageSize)> Processing;
};

using CustomImageMap = std::vector<TileMapping>;
struct CustomImage
{
    std::unordered_map<std::string, CustomImageMap> ImageMap;
    bool Outdated;
};
using CustomImages = std::unordered_map<std::string, CustomImage>;
