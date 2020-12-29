#include "image.h"

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

bool Image::LoadFromPng(const std::filesystem::path& file) {
	if (file.extension() != ".png") {
		return false;
	}

	mImpl = std::make_unique<ImageImpl>();

	{
		const std::uint32_t error = lodepng::decode(mImpl->Buffer, mImpl->Width, mImpl->Height, file.string(), LCT_RGBA, 8);
		if (error != 0) {
			return false;
		}
	}

	mImpl->Image = cv::Mat{ static_cast<int>(mImpl->Height), static_cast<int>(mImpl->Width), CV_8UC4, reinterpret_cast<int*>(mImpl->Buffer.data()) };

	return true;
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
	region.x *= tiling.TileSize.x;
	region.y *= tiling.TileSize.y;
	region.width *= tiling.TileSize.x;
	region.height *= tiling.TileSize.y;

	return GetSubImage(region);
}

void Image::Resize(ImageSize new_size) {
	cv::Mat resize_image;
	cv::resize(mImpl->Image, resize_image, cv::Size(new_size.x, new_size.y));
	mImpl->Image = std::move(resize_image);
	mImpl->Width = new_size.x;
	mImpl->Height = new_size.y;
}

void Image::Blit(const Image& source, ImageSubRegion region) {
	if (mImpl == nullptr) {
		return;
	}
	source.mImpl->Image.copyTo(mImpl->Image(cv::Rect(region.x, region.y, region.width, region.height)));
}
void Image::Blit(const Image& source, ImageTiling tiling, ImageSubRegion region) {
	region.x *= tiling.TileSize.x;
	region.y *= tiling.TileSize.y;
	region.width *= tiling.TileSize.x;
	region.height *= tiling.TileSize.y;

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

bool Image::IsSourceImage() const {
	return mImpl != nullptr && !mImpl->Buffer.empty();
}
std::span<std::uint8_t> Image::GetData() {
	if (mImpl == nullptr) {
		return {};
	}
	return mImpl->Buffer;
}
std::span<const std::uint8_t> Image::GetData() const {
	if (mImpl == nullptr) {
		return {};
	}
	return mImpl->Buffer;
}
