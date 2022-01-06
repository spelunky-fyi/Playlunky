#include "image_processing.h"

#include "../../res/resource_playlunky64.h"
#include "util/algorithms.h"
#include "util/format.h"
#include "util/on_scope_exit.h"

#include <Windows.h>

#pragma warning(push)
#pragma warning(disable : 5054)
#include <opencv2/imgproc.hpp>
#pragma warning(pop)

Image& make_square(Image& img)
{
    const std::uint32_t width = img.GetWidth();
    const std::uint32_t height = img.GetHeight();
    if (width > height)
    {
        const std::uint32_t delta = width - height;
        const std::int32_t top = -static_cast<std::int32_t>(std::ceil(static_cast<float>(delta) / 2.0f));
        img.Crop(ImageSubRegion{ .x{ 0 }, .y{ top }, .width{ width }, .height{ height + delta } });
    }
    else
    {
        const std::uint32_t delta = height - width;
        const std::int32_t left = -static_cast<std::int32_t>(std::ceil(static_cast<float>(delta) / 2.0f));
        img.Crop(ImageSubRegion{ .x{ left }, .y{ 0 }, .width{ width + delta }, .height{ height } });
    }

    return img;
}
Image& crop_to_bounding_box(Image& img)
{
    const auto bounding_rect = img.GetBoundingRect();
    if (bounding_rect.height == 0 || bounding_rect.width == 0)
    {
        return img;
    }

    img.Crop(bounding_rect);
    make_square(img);
    return img;
};

Image GenerateStickerPixelArt(Image input, ImageSize target_size)
{
    auto fix_transparent_pixels = [](Image& image, bool remove)
    {
        std::any backing_handle = image.GetBackingHandle();
        if (cv::Mat** cv_image_ptr = std::any_cast<cv::Mat*>(&backing_handle))
        {
            cv::Mat* cv_image = *cv_image_ptr;
            cv_image->forEach<cv::Vec4b>([remove](cv::Vec4b& pixel, [[maybe_unused]] const int position[])
                                         {
                                             if (remove && pixel[3] < 255)
                                             {
                                                 pixel[3] = 0;
                                             }
                                             else if (pixel[3] > 0)
                                             {
                                                 pixel[3] = 255;
                                             }
                                         });
        }
    };

    Image result = std::move(input);

    // Return empty image
    if (result.IsEmpty())
    {
        return result;
    }

    // Do KMeans
    {
        std::any backing_handle = result.GetBackingHandle();
        if (cv::Mat** cv_image_ptr = std::any_cast<cv::Mat*>(&backing_handle))
        {
            cv::Mat* cv_image = *cv_image_ptr;

            std::vector<cv::Mat> channels;
            cv::split(*cv_image, channels);

            std::vector<cv::Mat> color_channels{ channels[0], channels[1], channels[2] };
            cv::Mat color_only;
            cv::merge(color_channels, color_only);

            cv::Mat k_means_src;
            color_only.convertTo(k_means_src, CV_32F);
            k_means_src = k_means_src.reshape(1, static_cast<std::int32_t>(k_means_src.total()));

            constexpr std::int32_t k = 4;
            cv::Mat best_labels, centers, clustered;
            cv::kmeans(k_means_src, k, best_labels, cv::TermCriteria(cv::TermCriteria::Type::EPS | cv::TermCriteria::Type::MAX_ITER, 10, 1.0), 3, cv::KMEANS_PP_CENTERS, centers);

            centers = centers.reshape(3, centers.rows);
            k_means_src = k_means_src.reshape(0, k_means_src.rows);

            cv::Vec3f* k_means_src_data = k_means_src.ptr<cv::Vec3f>();
            for (std::int32_t i = 0; i < k_means_src.rows; i++)
            {
                std::int32_t center_id = best_labels.at<int>(i);
                k_means_src_data[i] = centers.at<cv::Vec3f>(center_id);
            }

            color_only = k_means_src.reshape(3, cv_image->rows);
            color_only.convertTo(color_only, CV_8U);

            color_channels.clear();
            cv::split(color_only, color_channels);
            channels[0] = color_channels[0];
            channels[1] = color_channels[1];
            channels[2] = color_channels[2];
            cv::merge(channels, *cv_image);
        }
    }

    // Crop to used bounding region and make square
    crop_to_bounding_box(result);
    fix_transparent_pixels(result, false);

    // Create Pixel Art
    result.Resize(::ImageSize{ .x{ 16 }, .y{ 16 } });
    result.Crop(ImageSubRegion{ .x{ -2 }, .y{ -2 }, .width{ 20 }, .height{ 20 } });

    // Increase Saturation
    {
        std::any backing_handle = result.GetBackingHandle();
        if (cv::Mat** cv_image_ptr = std::any_cast<cv::Mat*>(&backing_handle))
        {
            cv::Mat* cv_image = *cv_image_ptr;

            std::vector<cv::Mat> channels;
            cv::split(*cv_image, channels);

            std::vector<cv::Mat> color_channels{ channels[0], channels[1], channels[2] };
            cv::Mat color_only;
            cv::merge(color_channels, color_only);

            cv::Mat as_hsv;
            cv::cvtColor(color_only, as_hsv, cv::COLOR_RGB2HSV);
            as_hsv.forEach<cv::Vec3b>([](cv::Vec3b& pixel, [[maybe_unused]] const int position[])
                                      { pixel[1] = static_cast<uchar>(std::min(pixel[1] * 1.2f, 255.0f)); });
            cv::cvtColor(as_hsv, color_only, cv::COLOR_HSV2RGB);

            color_channels.clear();
            cv::split(color_only, color_channels);
            channels[0] = color_channels[0];
            channels[1] = color_channels[1];
            channels[2] = color_channels[2];
            cv::merge(channels, *cv_image);
        }
    }

    // Scale Up Pixel Art
    result.Resize(ImageSize{ .x{ 80 }, .y{ 80 } }, ScalingFilter::Nearest);
    fix_transparent_pixels(result, true);

    // Add Border and Drop Shadow
    {
        std::any backing_handle = result.GetBackingHandle();
        if (cv::Mat** cv_image_ptr = std::any_cast<cv::Mat*>(&backing_handle))
        {
            cv::Mat* cv_image = *cv_image_ptr;

            std::vector<cv::Mat> channels;
            cv::split(*cv_image, channels);

            cv::Mat& alpha_channel = channels[3];
            cv::Mat as_binary;
            cv::threshold(alpha_channel, as_binary, 128.0, 255.0, cv::THRESH_BINARY);

            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(as_binary, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

            cv::Mat contours_image = cv::Mat::zeros(cv_image->size(), cv_image->type());
            cv::drawContours(contours_image, contours, -1, cv::Scalar(255, 255, 255, 255), 8, cv::LINE_AA);

            cv::Mat drop_shadow_image = cv::Mat::zeros(cv_image->size(), cv_image->type());
            cv::drawContours(drop_shadow_image, contours, -1, cv::Scalar(0, 0, 0, 65), 8, cv::LINE_AA, cv::noArray(), std::numeric_limits<std::int32_t>::max(), cv::Point(1, 1));

            cv::Mat alpha_mask;
            cv::merge(std::vector<cv::Mat>{ alpha_channel, alpha_channel, alpha_channel, alpha_channel }, alpha_mask);
            cv::multiply(cv::Scalar::all(1.0f / 255.0f), alpha_mask, alpha_mask);
            cv::multiply(cv::Scalar::all(1.0f) - alpha_mask, contours_image, contours_image);

            cv::add(contours_image, *cv_image, *cv_image);
            cv::add(drop_shadow_image, *cv_image, *cv_image);
        }
    }

    // Resize to final size
    result.Resize(target_size);
    return result;
};

Image MakeCombinedMenuPetHeads(std::vector<std::pair<Image, std::filesystem::path>> pet_heads, ImageSize target_size)
{
    for (auto& [img, path] : pet_heads)
    {
        path = path.stem();
    }

    {
        using namespace std::string_view_literals;
        std::optional<Image> original_heads;
        for (auto& [pet_name, idx] : { std::pair{ "monty"sv, 0u }, std::pair{ "percy"sv, 1u }, std::pair{ "poochi"sv, 2u } })
        {
            if (!algo::contains_if(pet_heads, [pet_name](const auto& pair)
                                   { return pair.second.string().find(pet_name) != std::string::npos; }))
            {
                if (!original_heads.has_value())
                {
                    auto acquire_png_resource = [](LPSTR resource)
                    {
                        HMODULE this_module = GetModuleHandle("playlunky64.dll");
                        if (HRSRC png_resource = FindResource(this_module, resource, MAKEINTRESOURCE(PNG_FILE)))
                        {
                            if (HGLOBAL png_data = LoadResource(this_module, png_resource))
                            {
                                DWORD png_size = SizeofResource(this_module, png_resource);
                                return std::pair{ png_data, std::span<std::uint8_t>{ (std::uint8_t*)LockResource(png_data), png_size } };
                            }
                        }
                        return std::pair{ HGLOBAL{ NULL }, std::span<std::uint8_t>{} };
                    };
                    auto [pet_heads_res, pet_heads_png] = acquire_png_resource(MAKEINTRESOURCE(PET_HEADS));
                    OnScopeExit release_resources{ [pet_heads_res]
                                                   {
                                                       UnlockResource(pet_heads_res);
                                                   } };
                    original_heads.emplace();
                    original_heads->Load(pet_heads_png);
                }

                pet_heads.push_back(std::pair{
                    original_heads->GetSubImage(ImageTiling{ 128, 128 }, ImageSubRegion{ static_cast<std::int32_t>(idx), 0, 1, 1 }).Clone(),
                    std::filesystem::path{ pet_name } });
            }
        }
    }

    // Sort poochi -> percy -> monty
    std::sort(pet_heads.begin(), pet_heads.end(), [](const auto& lhs, const auto& rhs)
              { return lhs.second > rhs.second; });

    Image& poochi = crop_to_bounding_box(pet_heads[0].first);
    Image& percy = crop_to_bounding_box(pet_heads[1].first);
    Image& monty = crop_to_bounding_box(pet_heads[2].first);

    // Move in future subimage
    {
        const std::uint32_t width = poochi.GetWidth();
        const std::uint32_t height = poochi.GetHeight();
        poochi.Crop({
            static_cast<std::int32_t>(static_cast<float>(width) * -0.15f),
            0,
            width + static_cast<std::int32_t>(static_cast<float>(width) * 0.65f),
            height + static_cast<std::int32_t>(static_cast<float>(height) * 0.8f),
        });
        make_square(poochi);
    }
    {
        const std::uint32_t width = percy.GetWidth();
        const std::uint32_t height = percy.GetHeight();
        percy.Crop({
            static_cast<std::int32_t>(static_cast<float>(width) * -0.75f),
            static_cast<std::int32_t>(static_cast<float>(height) * -0.35f),
            width + static_cast<std::int32_t>(static_cast<float>(width) * 0.05f),
            height + static_cast<std::int32_t>(static_cast<float>(height) * 0.45f),
        });
        make_square(percy);
    }
    {
        const std::uint32_t width = monty.GetWidth();
        const std::uint32_t height = monty.GetHeight();
        monty.Crop({
            0,
            static_cast<std::int32_t>(static_cast<float>(height) * -0.8f),
            width + static_cast<std::int32_t>(static_cast<float>(width) * 0.8f),
            height,
        });
        make_square(monty);
    }

    // Scale to biggest
    {
        const std::uint32_t biggest_size = std::max(std::max(monty.GetWidth(), percy.GetWidth()), poochi.GetWidth());
        poochi.Resize({ biggest_size, biggest_size });
        percy.Resize({ biggest_size, biggest_size });
        monty.Resize({ biggest_size, biggest_size });
    }

    // Alpha blend images
    {
        std::any poochi_backing_handle = poochi.GetBackingHandle();
        std::any percy_backing_handle = percy.GetBackingHandle();
        std::any monty_backing_handle = monty.GetBackingHandle();
        cv::Mat** poochi_cv_image_ptr = std::any_cast<cv::Mat*>(&poochi_backing_handle);
        cv::Mat** percy_cv_image_ptr = std::any_cast<cv::Mat*>(&percy_backing_handle);
        cv::Mat** monty_cv_image_ptr = std::any_cast<cv::Mat*>(&monty_backing_handle);
        if (poochi_cv_image_ptr && percy_cv_image_ptr && monty_cv_image_ptr)
        {
            cv::Mat* monty_cv_image = *monty_cv_image_ptr;

            {
                cv::Mat* percy_cv_image = *percy_cv_image_ptr;

                std::vector<cv::Mat> monty_channels;
                cv::split(*monty_cv_image, monty_channels);
                cv::Mat& monty_alpha_channel = monty_channels[3];

                cv::Mat monty_alpha_mask;
                cv::merge(std::vector<cv::Mat>{ monty_alpha_channel, monty_alpha_channel, monty_alpha_channel, monty_alpha_channel }, monty_alpha_mask);
                cv::multiply(cv::Scalar::all(1.0f / 255.0f), monty_alpha_mask, monty_alpha_mask);
                cv::multiply(cv::Scalar::all(1.0f) - monty_alpha_mask, *percy_cv_image, *percy_cv_image);

                cv::add(*monty_cv_image, *percy_cv_image, *monty_cv_image);
            }

            {
                cv::Mat* poochi_cv_image = *poochi_cv_image_ptr;

                std::vector<cv::Mat> monty_channels;
                cv::split(*monty_cv_image, monty_channels);
                cv::Mat& monty_alpha_channel = monty_channels[3];

                cv::Mat monty_alpha_mask;
                cv::merge(std::vector<cv::Mat>{ monty_alpha_channel, monty_alpha_channel, monty_alpha_channel, monty_alpha_channel }, monty_alpha_mask);
                cv::multiply(cv::Scalar::all(1.0f / 255.0f), monty_alpha_mask, monty_alpha_mask);
                cv::multiply(cv::Scalar::all(1.0f) - monty_alpha_mask, *poochi_cv_image, *poochi_cv_image);

                cv::add(*monty_cv_image, *poochi_cv_image, *monty_cv_image);
            }
        }
    }

    monty.Resize(target_size);
    return std::move(monty);
}

inline auto get_lum = [](float r, float g, float b)
{
    return 0.3f * r + 0.59f * g + 0.11f * b;
};

inline auto set_lum = [](float r, float g, float b, float l)
{
    static auto clip_color = [](float r, float g, float b)
    {
        const float l = get_lum(r, g, b);
        const float n = std::min(r, std::min(g, b));
        const float x = std::max(r, std::max(g, b));

        if (n < 0)
        {
            r = l + (((r - l) * l) / (l - n));
            g = l + (((g - l) * l) / (l - n));
            b = l + (((b - l) * l) / (l - n));
        }

        if (x > 1)
        {
            r = l + (((r - l) * (1 - l)) / (x - l));
            g = l + (((g - l) * (1 - l)) / (x - l));
            b = l + (((b - l) * (1 - l)) / (x - l));
        }

        return std::tuple{ r, g, b };
    };

    const float d = l - get_lum(r, g, b);
    r += d;
    g += d;
    b += d;
    std::tie(r, g, b) = clip_color(r, g, b);

    return std::tuple{ r, g, b };
};

Image ColorBlend(Image color_image, Image target_image)
{
    if (color_image.GetWidth() != target_image.GetWidth() || color_image.GetHeight() != target_image.GetHeight())
    {
        color_image.Resize(ImageSize{ target_image.GetWidth(), target_image.GetHeight() });
    }

    std::any color_image_backing_handle = color_image.GetBackingHandle();
    std::any target_image_backing_handle = target_image.GetBackingHandle();
    cv::Mat** color_image_cv_image_ptr = std::any_cast<cv::Mat*>(&color_image_backing_handle);
    cv::Mat** target_image_cv_image_ptr = std::any_cast<cv::Mat*>(&target_image_backing_handle);

    if (color_image_cv_image_ptr && target_image_cv_image_ptr)
    {
        (*target_image_cv_image_ptr)->forEach<cv::Vec4b>([&](cv::Vec4b& pixel, const int position[])
                                                         {
                                                             const cv::Vec4b& color_pixel = (*color_image_cv_image_ptr)->at<cv::Vec4b>(position[0], position[1]);

                                                             const float color = get_lum(pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f);
                                                             const auto [fr, fg, fb] = set_lum(color_pixel[0] / 255.0f, color_pixel[1] / 255.0f, color_pixel[2] / 255.0f, color);
                                                             const auto r = static_cast<uchar>(std::clamp(fr * 255.0f, 0.0f, 255.0f));
                                                             const auto g = static_cast<uchar>(std::clamp(fg * 255.0f, 0.0f, 255.0f));
                                                             const auto b = static_cast<uchar>(std::clamp(fb * 255.0f, 0.0f, 255.0f));

                                                             const float color_alpha = color_pixel[3] / 255.0f;
                                                             pixel[0] = static_cast<uchar>(pixel[0] * (1.0f - color_alpha) + r * color_alpha);
                                                             pixel[1] = static_cast<uchar>(pixel[1] * (1.0f - color_alpha) + g * color_alpha);
                                                             pixel[2] = static_cast<uchar>(pixel[2] * (1.0f - color_alpha) + b * color_alpha);
                                                         });

        return target_image;
    }
    return {};
}

Image LuminanceBlend(Image luminance_image, Image target_image)
{
    if (luminance_image.GetWidth() != target_image.GetWidth() || luminance_image.GetHeight() != target_image.GetHeight())
    {
        luminance_image.Resize(ImageSize{ target_image.GetWidth(), target_image.GetHeight() });
    }

    std::any luminance_image_backing_handle = luminance_image.GetBackingHandle();
    std::any target_image_backing_handle = target_image.GetBackingHandle();
    cv::Mat** luminance_image_cv_image_ptr = std::any_cast<cv::Mat*>(&luminance_image_backing_handle);
    cv::Mat** target_image_cv_image_ptr = std::any_cast<cv::Mat*>(&target_image_backing_handle);

    if (luminance_image_cv_image_ptr && target_image_cv_image_ptr)
    {
        (*target_image_cv_image_ptr)->forEach<cv::Vec4b>([&](cv::Vec4b& pixel, const int position[])
                                                         {
                                                             const cv::Vec4b& luminance_pixel = (*luminance_image_cv_image_ptr)->at<cv::Vec4b>(position[0], position[1]);
                                                             const float luminance = get_lum(luminance_pixel[0] / 255.0f, luminance_pixel[1] / 255.0f, luminance_pixel[2] / 255.0f);
                                                             const auto [fr, fg, fb] = set_lum(pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, luminance);
                                                             const auto r = static_cast<uchar>(std::clamp(fr * 255.0f, 0.0f, 255.0f));
                                                             const auto g = static_cast<uchar>(std::clamp(fg * 255.0f, 0.0f, 255.0f));
                                                             const auto b = static_cast<uchar>(std::clamp(fb * 255.0f, 0.0f, 255.0f));

                                                             const float luminance_alpha = luminance_pixel[3] / 255.0f;
                                                             pixel[0] = static_cast<uchar>(pixel[0] * (1.0f - luminance_alpha) + r * luminance_alpha);
                                                             pixel[1] = static_cast<uchar>(pixel[1] * (1.0f - luminance_alpha) + g * luminance_alpha);
                                                             pixel[2] = static_cast<uchar>(pixel[2] * (1.0f - luminance_alpha) + b * luminance_alpha);
                                                         });

        return target_image;
    }
    return {};
}

Image ReplaceColor(Image input_image, ColorRGB8 source_color, ColorRGB8 target_color)
{
    std::any image_backing_handle = input_image.GetBackingHandle();
    cv::Mat** image_cv_image_ptr = std::any_cast<cv::Mat*>(&image_backing_handle);

    if (image_cv_image_ptr)
    {
        (*image_cv_image_ptr)->forEach<cv::Vec4b>([&](cv::Vec4b& cv_pixel, [[maybe_unused]] const int position[])
                                                  {
                                                      ColorRGB8& pixel = reinterpret_cast<ColorRGB8&>(cv_pixel);
                                                      if (pixel == source_color)
                                                      {
                                                          pixel = target_color;
                                                      }
                                                  });
    }
    return input_image;
}
Image ReplaceColors(Image input_image, const std::vector<ColorRGB8>& source_colors, const std::vector<ColorRGB8>& target_colors)
{

    std::any image_backing_handle = input_image.GetBackingHandle();
    cv::Mat** image_cv_image_ptr = std::any_cast<cv::Mat*>(&image_backing_handle);

    if (image_cv_image_ptr)
    {
        (*image_cv_image_ptr)->forEach<cv::Vec4b>([&](cv::Vec4b& cv_pixel, [[maybe_unused]] const int position[])
                                                  {
                                                      ColorRGB8& pixel = reinterpret_cast<ColorRGB8&>(cv_pixel);
                                                      for (size_t i = 0; i < source_colors.size(); i++)
                                                      {
                                                          const ColorRGB8& source_color = source_colors[i];
                                                          if (pixel == source_color)
                                                          {
                                                              pixel = target_colors[i];
                                                              return;
                                                          }
                                                      }
                                                  });
    }
    return input_image;
}

std::vector<ColorRGB8> GetUniqueColors(const Image& image, std::size_t max_numbers)
{
    std::vector<ColorRGB8> unique_colors;

    std::any image_backing_handle = image.GetBackingHandle();
    const cv::Mat** image_cv_image_ptr = std::any_cast<const cv::Mat*>(&image_backing_handle);

    if (image_cv_image_ptr)
    {
        for (const cv::Vec4b& cv_pixel : cv::Mat_<cv::Vec4b>(**image_cv_image_ptr))
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
    }

    return unique_colors;
}
