#pragma once

#include <any>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>

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

    bool LoadInfoFromPng(const std::filesystem::path& file);
    bool LoadFromPng(const std::filesystem::path& file);
    bool LoadFromPng(const std::span<std::uint8_t>& data);

    Image Copy();
    Image Clone() const;

    bool ContainsSubRegion(ImageSubRegion region) const;

    Image GetSubImage(ImageSubRegion region);
    Image GetSubImage(ImageTiling tiling, ImageSubRegion region);

    void Blit(const Image& source, ImageSubRegion region);
    void Blit(const Image& source, ImageTiling tiling, ImageSubRegion region);

    void Resize(ImageSize new_size, ScalingFilter filter = ScalingFilter::Linear);

    void Crop(ImageSubRegion region);

    bool IsEmpty() const;

    std::uint32_t GetWidth() const;
    std::uint32_t GetHeight() const;
    ImageSubRegion GetBoundingRect() const;

    bool IsSourceImage() const;
    std::span<std::uint8_t> GetData();
    std::span<const std::uint8_t> GetData() const;

    const std::any GetBackingHandle() const;
    std::any GetBackingHandle();

    void DebugShow() const;

  private:
    struct ImageImpl;
    std::unique_ptr<ImageImpl> mImpl;
};
