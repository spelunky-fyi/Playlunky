#include "image.h"

#include "util/span_util.h"

#include <array>
#include <fstream>

#pragma warning(push)
#pragma warning(disable : 5054)
#include <opencv2/imgproc.hpp>
#pragma warning(pop)

#include <lodepng.h>

struct Image::ImageImpl {
	cv::Mat Image;
	std::vector<std::uint8_t> Buffer;
	std::uint32_t Width;
	std::uint32_t Height;
};

Image::Image() = default;
Image::Image(Image&&) noexcept = default;
Image& Image::operator=(Image&&) noexcept = default;
Image::~Image() = default;

bool Image::LoadInfoFromPng(const std::filesystem::path& file)
{
	if (file.extension() != ".png") {
		return false;
	}

	namespace fs = std::filesystem;
	if (fs::exists(file) && fs::is_regular_file(file)) {
		mImpl = std::make_unique<ImageImpl>();

		std::ifstream png_file(file, std::ios::binary);

		std::array<unsigned char, 128> first_chunk;
		png_file.read((char*)first_chunk.data(), first_chunk.size());

		lodepng::State state;
		return lodepng_inspect(&mImpl->Width, &mImpl->Height, &state, first_chunk.data(), first_chunk.size()) == 0;
	}

	return false;
}

bool Image::LoadFromPng(const std::filesystem::path& file) {
	if (file.extension() != ".png") {
		return false;
	}

	if (mImpl == nullptr)
	{
		mImpl = std::make_unique<ImageImpl>();
	}

	{
		const std::uint32_t error = lodepng::decode(mImpl->Buffer, mImpl->Width, mImpl->Height, file.string(), LCT_RGBA, 8);
		if (error != 0) {
			return false;
		}
	}

	{
		struct ColorRGBA8 {
			std::uint8_t R;
			std::uint8_t G;
			std::uint8_t B;
			std::uint8_t A;
		};
		auto image = span::bit_cast<ColorRGBA8>(mImpl->Buffer);
		for (ColorRGBA8& pixel : image) {
			if (pixel.A == 0) {
				pixel = ColorRGBA8{};
			}
		}
	}

	mImpl->Image = cv::Mat{ static_cast<int>(mImpl->Height), static_cast<int>(mImpl->Width), CV_8UC4, reinterpret_cast<int*>(mImpl->Buffer.data()) };

	return true;
}

Image Image::Copy() {
	return GetSubImage(ImageSubRegion{ 
			.x{ 0 },
			.y{ 0 },
			.width{ mImpl->Width },
			.height{ mImpl->Height }
		});
}

bool Image::ContainsSubRegion(ImageSubRegion region) const {
	const std::int32_t right = region.x + static_cast<std::int32_t>(region.width);
	const std::int32_t bottom = region.y + static_cast<std::int32_t>(region.height);
	return region.x >= 0
		&& region.y >= 0
		&& right <= static_cast<std::int32_t>(mImpl->Width)
		&& bottom <= static_cast<std::int32_t>(mImpl->Height);
}

Image Image::GetSubImage(ImageSubRegion region) {
	if (mImpl == nullptr) {
		return {};
	}

	Image sub_image;
	sub_image.mImpl = std::make_unique<ImageImpl>();
	sub_image.mImpl->Width = region.width;
	sub_image.mImpl->Height = region.height;
	sub_image.mImpl->Image = mImpl->Image(cv::Rect(region.x, region.y, region.width, region.height));

	return sub_image;
}
Image Image::GetSubImage(ImageTiling tiling, ImageSubRegion region) {
	const TileDimensions this_tile_size = tiling.ThisTileSize.value_or(tiling.TileSize);

	region.x *= tiling.TileSize.x;
	region.y *= tiling.TileSize.y;
	region.width *= this_tile_size.x;
	region.height *= this_tile_size.y;

	return GetSubImage(region);
}

void Image::Resize(ImageSize new_size, ScalingFilter filter) {
	cv::Mat resize_image;
	cv::InterpolationFlags inter = [filter]() {
		switch (filter) {
		default:
		case ScalingFilter::Linear:
			return cv::INTER_LINEAR;
		case ScalingFilter::Nearest:
			return cv::INTER_NEAREST;
		}
	}();
	cv::resize(mImpl->Image, resize_image, cv::Size(new_size.x, new_size.y), 0.0, 0.0, inter);
	mImpl->Image = std::move(resize_image);
	mImpl->Width = new_size.x;
	mImpl->Height = new_size.y;
}

void Image::Crop(ImageSubRegion region) {
	const std::int32_t right = region.x + static_cast<std::int32_t>(region.width);
	const std::int32_t bottom = region.y + static_cast<std::int32_t>(region.height);
	if (region.x < 0 || region.y < 0 || right > static_cast<std::int32_t>(mImpl->Width) || bottom > static_cast<std::int32_t>(mImpl->Height)) {
		static const cv::Scalar transparent(0, 0, 0, 0);

		cv::Mat larger_image;
		cv::copyMakeBorder(mImpl->Image,
			larger_image,
			std::max(-region.y, 0),
			std::max(bottom - static_cast<std::int32_t>(mImpl->Height), 0),
			std::max(-region.x, 0),
			std::max(right - static_cast<std::int32_t>(mImpl->Width), 0),
			cv::BORDER_CONSTANT,
			transparent);
		mImpl->Image = larger_image;
	}
	else {
		mImpl->Image = mImpl->Image(cv::Rect(region.x, region.y, region.width, region.height));
	}
	mImpl->Width = region.width;
	mImpl->Height = region.height;
}

void Image::Blit(const Image& source, ImageSubRegion region) {
	if (mImpl == nullptr) {
		return;
	}
	source.mImpl->Image.copyTo(mImpl->Image(cv::Rect(region.x, region.y, region.width, region.height)));
}
void Image::Blit(const Image& source, ImageTiling tiling, ImageSubRegion region) {
	const TileDimensions this_tile_size = tiling.ThisTileSize.value_or(tiling.TileSize);

	region.x *= tiling.TileSize.x;
	region.y *= tiling.TileSize.y;
	region.width *= this_tile_size.x;
	region.height *= this_tile_size.y;

	Blit(source, region);
}

bool Image::IsEmpty() const {
	return mImpl == nullptr || mImpl->Image.empty() || cv::mean(mImpl->Image) == cv::Scalar{ 0 };
}

std::uint32_t Image::GetWidth() const {
	if (mImpl == nullptr) {
		return 0;
	}
	return mImpl->Width;
}
std::uint32_t Image::GetHeight() const {
	if (mImpl == nullptr) {
		return 0;
	}
	return mImpl->Height;
}
ImageSubRegion Image::GetBoundingRect() const {
	std::vector<cv::Mat> channels;
	cv::split(mImpl->Image, channels);
	cv::Rect rect = cv::boundingRect(channels[3]);
	return ImageSubRegion{
		.x{ rect.x },
		.y{ rect.y },
		.width{ static_cast<std::uint32_t>(rect.width) },
		.height{ static_cast<std::uint32_t>(rect.height) }
	};
}

bool Image::IsSourceImage() const {
	return mImpl != nullptr && !mImpl->Buffer.empty();
}
std::span<std::uint8_t> Image::GetData() {
	if (mImpl == nullptr) {
		return {};
	}
	return span::bit_cast<std::uint8_t>(std::span{ mImpl->Image.data, static_cast<std::size_t>(mImpl->Image.dataend - mImpl->Image.datastart) });
}
std::span<const std::uint8_t> Image::GetData() const {
	if (mImpl == nullptr) {
		return {};
	}
	return span::bit_cast<const std::uint8_t>(std::span{ mImpl->Image.data, static_cast<std::size_t>(mImpl->Image.dataend - mImpl->Image.datastart) });
}

const std::any Image::GetBackingHandle() const {
	return &mImpl->Image;
}
std::any Image::GetBackingHandle() {
	return &mImpl->Image;
}
