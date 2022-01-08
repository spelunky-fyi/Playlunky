#include "image.h"

#include "log.h"
#include "util/algorithms.h"
#include "util/format.h"
#include "util/span_util.h"

#include <array>
#include <fstream>

#pragma warning(push)
#pragma warning(disable : 5054)
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#pragma warning(pop)

struct Image::ImageImpl
{
    cv::Mat Image;
    std::uint32_t Width;
    std::uint32_t Height;
};

Image::Image() = default;
Image::Image(Image&&) noexcept = default;
Image& Image::operator=(Image&&) noexcept = default;
Image::~Image() = default;

bool Image::Load(const std::filesystem::path& file)
{
    if (mImpl == nullptr)
    {
        mImpl = std::make_unique<ImageImpl>();
    }

    mImpl->Image = cv::imread(std::filesystem::absolute(file).string(), cv::IMREAD_UNCHANGED);
    if (mImpl->Image.empty())
    {
        mImpl = nullptr;
        return false;
    }

    mImpl->Width = mImpl->Image.cols;
    mImpl->Height = mImpl->Image.rows;

    if (!ConvertToRGBA())
    {
        mImpl = nullptr;
        return false;
    }

    mImpl->Image.forEach<cv::Vec4b>([](cv::Vec4b& pixel, [[maybe_unused]] const int position[])
                                    {
                                        float alpha = (float)pixel[3] / 255.0f;
                                        pixel[0] = (uchar)((float)pixel[0] * alpha);
                                        pixel[1] = (uchar)((float)pixel[1] * alpha);
                                        pixel[2] = (uchar)((float)pixel[2] * alpha);
                                    });

    return true;
}
bool Image::Load(const std::span<std::uint8_t>& data)
{
    if (mImpl == nullptr)
    {
        mImpl = std::make_unique<ImageImpl>();
    }

    mImpl->Image = cv::imdecode(cv::InputArray{ data.data(), static_cast<int>(data.size()) }, cv::IMREAD_UNCHANGED);
    if (mImpl->Image.empty())
    {
        mImpl = nullptr;
        return false;
    }

    mImpl->Width = mImpl->Image.cols;
    mImpl->Height = mImpl->Image.rows;

    if (!ConvertToRGBA())
    {
        mImpl = nullptr;
        return false;
    }

    return true;
}
void Image::LoadRawData(const std::span<std::uint8_t>& data, std::uint32_t width, std::uint32_t height)
{
    if (mImpl == nullptr)
    {
        mImpl = std::make_unique<ImageImpl>();
    }

    mImpl->Image = cv::Mat{ static_cast<int>(height), static_cast<int>(width), CV_8UC4, reinterpret_cast<int*>(data.data()) };

    mImpl->Width = width;
    mImpl->Height = height;
}

bool Image::Write(const std::filesystem::path& file)
{
    if (mImpl == nullptr)
    {
        return false;
    }

    {
        const auto parent_path = file.parent_path();
        if (std::filesystem::exists(parent_path) && !std::filesystem::is_directory(parent_path))
        {
            LogError("Can not write file {}, its parent directory exists and is not a directory...", file.string());
        }
        else if (!std::filesystem::exists(parent_path))
        {
            std::filesystem::create_directories(parent_path);
        }
    }

    cv::Mat bgra_image;
    cv::cvtColor(mImpl->Image, bgra_image, cv::COLOR_RGBA2BGRA);
    return cv::imwrite(file.string(), bgra_image);
}

Image Image::Copy()
{
    return GetSubImage(ImageSubRegion{
        .x{ 0 },
        .y{ 0 },
        .width{ mImpl->Width },
        .height{ mImpl->Height } });
}
Image Image::Clone() const
{
    Image clone;
    clone.mImpl = std::make_unique<ImageImpl>();
    mImpl->Image.copyTo(clone.mImpl->Image);
    clone.mImpl->Width = mImpl->Width;
    clone.mImpl->Height = mImpl->Height;
    return clone;
}

bool Image::ContainsSubRegion(ImageSubRegion region) const
{
    const std::int32_t right = region.x + static_cast<std::int32_t>(region.width);
    const std::int32_t bottom = region.y + static_cast<std::int32_t>(region.height);
    return region.x >= 0 && region.y >= 0 && right <= static_cast<std::int32_t>(mImpl->Width) && bottom <= static_cast<std::int32_t>(mImpl->Height);
}

Image Image::CloneSubImage(ImageSubRegion region) const
{
    return const_cast<Image*>(this)->GetSubImage(region).Clone();
}
Image Image::CloneSubImage(ImageTiling tiling, ImageSubRegion region) const
{
    return const_cast<Image*>(this)->GetSubImage(tiling, region).Clone();
}

Image Image::GetSubImage(ImageSubRegion region) const
{
    if (mImpl == nullptr)
    {
        return {};
    }

    Image sub_image;
    sub_image.mImpl = std::make_unique<ImageImpl>();
    sub_image.mImpl->Width = region.width;
    sub_image.mImpl->Height = region.height;
    sub_image.mImpl->Image = mImpl->Image(cv::Rect(region.x, region.y, region.width, region.height));

    return sub_image;
}
Image Image::GetSubImage(ImageTiling tiling, ImageSubRegion region) const
{
    const TileDimensions this_tile_size = tiling.ThisTileSize.value_or(tiling.TileSize);

    region.x *= tiling.TileSize.x;
    region.y *= tiling.TileSize.y;
    region.width *= this_tile_size.x;
    region.height *= this_tile_size.y;

    return GetSubImage(region);
}

std::pair<Image, ImageSubRegion> Image::GetFirstSprite() const
{
    std::vector<std::pair<Image, ImageSubRegion>> all_sprites = GetSprites();
    if (all_sprites.empty())
    {
        return {};
    }
    return std::move(all_sprites.front());
}
std::vector<std::pair<Image, ImageSubRegion>> Image::GetSprites() const
{
    if (mImpl == nullptr)
    {
        return {};
    }

    std::vector<cv::Mat> channels;
    cv::split(mImpl->Image, channels);

    cv::Mat binary;
    cv::threshold(channels[3], binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    std::vector<cv::Mat> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty())
    {
        return {};
    }

    std::vector<std::pair<cv::Mat, ImageSubRegion>> contours_and_regions;
    for (cv::Mat& contour : contours)
    {
        cv::Rect rect = cv::boundingRect(contour);
        const ImageSubRegion sub_region{
            .x{ rect.x },
            .y{ rect.y },
            .width{ static_cast<std::uint32_t>(rect.width) },
            .height{ static_cast<std::uint32_t>(rect.height) }
        };
        contours_and_regions.push_back({ std::move(contour), sub_region });
    }
    std::sort(contours_and_regions.begin(), contours_and_regions.end(), [](const auto& lhs, const auto& rhs)
              { return lhs.second.x * lhs.second.x + lhs.second.y * lhs.second.y < rhs.second.x * rhs.second.x + rhs.second.y * rhs.second.y; });

    std::vector<std::pair<Image, ImageSubRegion>> images;
    for (const auto& [contour, sub_region] : contours_and_regions)
    {
        Image sub_image = GetSubImage(sub_region);
        std::vector<ColorRGB8> unique_colors = sub_image.GetUniqueColors();
        if (sub_image.mImpl != nullptr)
        {
            images.push_back({ std::move(sub_image), sub_region });
        }
    }
    return images;
}

std::vector<ColorRGB8> Image::GetUniqueColors(std::size_t max_numbers) const
{
    if (mImpl == nullptr)
    {
        return {};
    }

    std::vector<ColorRGB8> unique_colors;
    for (const cv::Vec4b& cv_pixel : cv::Mat_<cv::Vec4b>(mImpl->Image))
    {
        const ColorRGB8& pixel = reinterpret_cast<const ColorRGB8&>(cv_pixel);
        if (cv_pixel[3] == 255 && !algo::contains(unique_colors, pixel))
        {
            unique_colors.push_back(pixel);
            if (unique_colors.size() >= max_numbers)
            {
                break;
            }
        }
    }
    return unique_colors;
}

void Image::Resize(ImageSize new_size, ScalingFilter filter)
{
    cv::Mat resize_image;
    cv::InterpolationFlags inter = [filter]()
    {
        switch (filter)
        {
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

void Image::Crop(ImageSubRegion region)
{
    const std::int32_t right = region.x + static_cast<std::int32_t>(region.width);
    const std::int32_t bottom = region.y + static_cast<std::int32_t>(region.height);
    if (region.x < 0 || region.y < 0 || right > static_cast<std::int32_t>(mImpl->Width) || bottom > static_cast<std::int32_t>(mImpl->Height))
    {
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
    else
    {
        mImpl->Image = mImpl->Image(cv::Rect(region.x, region.y, region.width, region.height));
    }
    mImpl->Width = mImpl->Image.cols;
    mImpl->Height = mImpl->Image.rows;
}

void Image::Blit(const Image& source, ImageSubRegion region)
{
    if (mImpl == nullptr)
    {
        return;
    }
    source.mImpl->Image.copyTo(mImpl->Image(cv::Rect(region.x, region.y, region.width, region.height)));
}
void Image::Blit(const Image& source, ImageTiling tiling, ImageSubRegion region)
{
    const TileDimensions this_tile_size = tiling.ThisTileSize.value_or(tiling.TileSize);

    region.x *= tiling.TileSize.x;
    region.y *= tiling.TileSize.y;
    region.width *= this_tile_size.x;
    region.height *= this_tile_size.y;

    Blit(source, region);
}

bool Image::IsEmpty() const
{
    return mImpl == nullptr || mImpl->Image.empty() || cv::mean(mImpl->Image) == cv::Scalar{ 0 };
}

std::uint32_t Image::GetWidth() const
{
    if (mImpl == nullptr)
    {
        return 0;
    }
    return mImpl->Width;
}
std::uint32_t Image::GetHeight() const
{
    if (mImpl == nullptr)
    {
        return 0;
    }
    return mImpl->Height;
}
ImageSubRegion Image::GetBoundingRect() const
{
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

std::span<std::uint8_t> Image::GetData()
{
    if (mImpl == nullptr)
    {
        return {};
    }
    return span::bit_cast<std::uint8_t>(std::span{ mImpl->Image.data, static_cast<std::size_t>(mImpl->Image.dataend - mImpl->Image.datastart) });
}
std::span<const std::uint8_t> Image::GetData() const
{
    if (mImpl == nullptr)
    {
        return {};
    }
    return span::bit_cast<const std::uint8_t>(std::span{ mImpl->Image.data, static_cast<std::size_t>(mImpl->Image.dataend - mImpl->Image.datastart) });
}

const std::any Image::GetBackingHandle() const
{
    return static_cast<const cv::Mat*>(&mImpl->Image);
}
std::any Image::GetBackingHandle()
{
    return &mImpl->Image;
}

void Image::DebugShow() const
{
    try
    {
        cv::Mat bgra_image;
        cv::cvtColor(mImpl->Image, bgra_image, cv::COLOR_RGBA2BGRA);
        cv::imshow("some", bgra_image);
        cv::waitKey();
    }
    catch (cv::Exception& e)
    {
        fmt::print("{}", e.what());
    }
}

bool Image::ConvertToRGBA()
{
    switch (mImpl->Image.channels())
    {
    default:
    case 0:
    case 1:
    case 2:
        return false;
    case 3:
    {
        std::vector<cv::Mat> channels;
        if (mImpl->Image.type() != CV_8UC3)
        {
            double alpha = 1.0;
            double beta = 0.0;
            switch (mImpl->Image.depth())
            {
            default:
            case CV_8U:
                break;
            case CV_8S:
                beta = static_cast<double>(CHAR_MIN);
                break;
            case CV_16U:
                alpha = 1.0 / UCHAR_MAX;
                break;
            case CV_16S:
                alpha = 1.0 / UCHAR_MAX;
                beta = static_cast<double>(CHAR_MIN);
                break;
            case CV_32S:
                alpha = 1.0 / USHRT_MAX;
                beta = static_cast<double>(CHAR_MIN);
                break;
            case CV_32F:
                alpha = static_cast<double>(UCHAR_MAX);
                break;
            case CV_64F:
                alpha = static_cast<double>(UCHAR_MAX);
                break;
            }
            mImpl->Image.convertTo(mImpl->Image, CV_8UC3, alpha, beta);
        }
        cv::split(mImpl->Image, channels);
        std::swap(channels[0], channels[2]);
        cv::Mat alpha(channels[0].size(), channels[0].type());
        alpha = cv::Scalar(255);
        channels.push_back(alpha);
        cv::merge(channels, mImpl->Image);
        return true;
    }
    case 4:
    {
        std::vector<cv::Mat> channels;
        if (mImpl->Image.type() != CV_8UC4)
        {
            double alpha = 1.0;
            double beta = 0.0;
            switch (mImpl->Image.depth())
            {
            default:
            case CV_8U:
                break;
            case CV_8S:
                beta = static_cast<double>(CHAR_MIN);
                break;
            case CV_16U:
                alpha = 1.0 / UCHAR_MAX;
                break;
            case CV_16S:
                alpha = 1.0 / UCHAR_MAX;
                beta = static_cast<double>(CHAR_MIN);
                break;
            case CV_32S:
                alpha = 1.0 / USHRT_MAX;
                beta = static_cast<double>(CHAR_MIN);
                break;
            case CV_32F:
                alpha = static_cast<double>(UCHAR_MAX);
                break;
            case CV_64F:
                alpha = static_cast<double>(UCHAR_MAX);
                break;
            }
            mImpl->Image.convertTo(mImpl->Image, CV_8UC4, alpha, beta);
        }
        cv::split(mImpl->Image, channels);
        std::swap(channels[0], channels[2]);
        cv::merge(channels, mImpl->Image);
        return true;
    }
    }
}
