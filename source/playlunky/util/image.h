#pragma once

#include <any>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>

#include "util/color.h"

struct TileDimensions
{
    std::uint32_t x;
    std::uint32_t y;

    bool operator==(const TileDimensions&) const = default;
};

struct ImageTiling
{
    TileDimensions TileSize;
    std::optional<TileDimensions> ThisTileSize;
};

struct ImageSubRegion
{
    std::int32_t x;
    std::int32_t y;
    std::uint32_t width;
    std::uint32_t height;
};

struct ImageSize
{
    std::uint32_t x;
    std::uint32_t y;
};

enum class ScalingFilter
{
    Linear,
    Nearest
};

class Image
{
  public:
    Image();
    Image(const Image&) = delete;
    Image(Image&&) noexcept;
    Image& operator=(const Image&) = delete;
    Image& operator=(Image&&) noexcept;
    ~Image();

    bool Load(const std::filesystem::path& file);
    bool Load(const std::span<std::uint8_t>& data);
    void LoadRawData(const std::span<std::uint8_t>& data, std::uint32_t width, std::uint32_t height);

    bool Write(const std::filesystem::path& file);

    // Create a shallow copy of the image, shares the data with the original image
    Image Copy();
    // Create a deep copy of the image, owns its own data
    Image Clone() const;

    bool ContainsSubRegion(ImageSubRegion region) const;

    Image CloneSubImage(ImageSubRegion region) const;
    Image CloneSubImage(ImageTiling tiling, ImageSubRegion region) const;

    Image GetSubImage(ImageSubRegion region) const;
    Image GetSubImage(ImageTiling tiling, ImageSubRegion region) const;

    std::pair<Image, ImageSubRegion> GetFirstSprite() const;
    std::vector<std::pair<Image, ImageSubRegion>> GetSprites() const;

    std::optional<ColorRGB8> GetFirstColor() const;
    std::vector<ColorRGB8> GetUniqueColors(std::size_t max_numbers = 16) const;

    void Blit(const Image& source, ImageSubRegion region);
    void Blit(const Image& source, ImageTiling tiling, ImageSubRegion region);

    void Resize(ImageSize new_size, ScalingFilter filter = ScalingFilter::Linear);

    void Crop(ImageSubRegion region);

    bool IsEmpty() const;

    std::uint32_t GetWidth() const;
    std::uint32_t GetHeight() const;
    ImageSubRegion GetBoundingRect() const;

    std::span<std::uint8_t> GetData();
    std::span<const std::uint8_t> GetData() const;

    const std::any GetBackingHandle() const;
    std::any GetBackingHandle();

    void DebugShow() const;
    void DebugShowWith(const Image& other) const;

  private:
    bool ConvertToRGBA();

    struct ImageImpl;
    std::unique_ptr<ImageImpl> mImpl;
};
