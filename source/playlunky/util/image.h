#pragma once

#include <filesystem>
#include <memory>
#include <span>

struct ImageTiling {
	struct { std::uint32_t x, y; } TileSize;
};

struct ImageSubRegion {
	std::uint32_t x;
	std::uint32_t y;
	std::uint32_t width;
	std::uint32_t height;
};

struct ImageSize {
	std::uint32_t x;
	std::uint32_t y;
};

class Image {
public:
	Image();
	Image(const Image&) = delete;
	Image(Image&&) noexcept;
	Image& operator=(const Image&) = delete;
	Image& operator=(Image&&) noexcept;
	~Image();

	bool LoadFromPng(const std::filesystem::path& file);

	Image GetSubImage(ImageSubRegion region);
	Image GetSubImage(ImageTiling tiling, ImageSubRegion region);

	void Blit(const Image& source, ImageSubRegion region);
	void Blit(const Image& source, ImageTiling tiling, ImageSubRegion region);

	void Resize(ImageSize new_size);

	bool IsEmpty() const;

	std::uint32_t GetWidth() const;
	std::uint32_t GetHeight() const;

	bool IsSourceImage() const;
	std::span<std::uint8_t> GetData();
	std::span<const std::uint8_t> GetData() const;

private:
	struct ImageImpl;
	std::unique_ptr<ImageImpl> mImpl;
};
