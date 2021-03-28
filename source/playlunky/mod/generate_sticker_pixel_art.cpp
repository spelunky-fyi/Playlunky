#include "generate_sticker_pixel_art.h"

#pragma warning(push)
#pragma warning(disable : 5054)
#include <opencv2/imgproc.hpp>
#pragma warning(pop)

Image GenerateStickerPixelArt(Image input, ImageSize target_size) {
	auto fix_transparent_pixels = [](Image& image, bool remove) {
		std::any backing_handle = image.GetBackingHandle();
		if (cv::Mat** cv_image_ptr = std::any_cast<cv::Mat*>(&backing_handle)) {
			cv::Mat* cv_image = *cv_image_ptr;
			cv_image->forEach<cv::Vec4b>([remove](cv::Vec4b& pixel, [[maybe_unused]] const int position[]) {
					if (remove && pixel[3] < 255) {
						pixel[3] = 0;
					}
					else if (pixel[3] > 0) {
						pixel[3] = 255;
					}
				});
		}
	};

	Image result = std::move(input);

	// Do KMeans
	{
		std::any backing_handle = result.GetBackingHandle();
		if (cv::Mat** cv_image_ptr = std::any_cast<cv::Mat*>(&backing_handle)) {
			cv::Mat* cv_image = *cv_image_ptr;

			std::vector<cv::Mat> channels;
			cv::split(*cv_image, channels);

			std::vector<cv::Mat> color_channels{ channels[0], channels[1], channels[2] };
			cv::Mat color_only;
			cv::merge(color_channels, color_only);

			cv::Mat k_means_src;
			color_only.convertTo(k_means_src, CV_32F);
			k_means_src = k_means_src.reshape(1, static_cast<std::int32_t>(k_means_src.total()));

			const std::int32_t k = 8;
			cv::Mat best_labels, centers, clustered;
			cv::kmeans(k_means_src, k, best_labels,
				cv::TermCriteria(cv::TermCriteria::Type::EPS | cv::TermCriteria::Type::MAX_ITER, 10, 1.0),
				3, cv::KMEANS_PP_CENTERS, centers);

			centers = centers.reshape(3, centers.rows);
			k_means_src = k_means_src.reshape(0, k_means_src.rows);

			cv::Vec3f* k_means_src_data = k_means_src.ptr<cv::Vec3f>();
			for (std::int32_t i = 0; i < k_means_src.rows; i++) {
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
	result.Crop(result.GetBoundingRect());
	fix_transparent_pixels(result, false);
	{
		const std::uint32_t width = result.GetWidth();
		const std::uint32_t height = result.GetHeight();
		if (width > height) {
			const std::uint32_t delta = width - height;
			const std::int32_t top = -static_cast<std::int32_t>(std::ceil(static_cast<float>(delta) / 2.0f));
			result.Crop(ImageSubRegion{ .x{ 0 }, .y{ top }, .width{ width }, .height{ height + delta } });
		}
		else {
			const std::uint32_t delta = height - width;
			const std::int32_t left = -static_cast<std::int32_t>(std::ceil(static_cast<float>(delta) / 2.0f));
			result.Crop(ImageSubRegion{ .x{ left }, .y{ 0 }, .width{ width + delta }, .height{ height } });
		}
	}

	// Create Pixel Art
	result.Resize(::ImageSize{ .x{ 16 }, .y{ 16 } });
	result.Crop(ImageSubRegion{ .x{ -2 }, .y{ -2 }, .width{ 20 }, .height{ 20 } });

	// Increase Saturation
	{
		std::any backing_handle = result.GetBackingHandle();
		if (cv::Mat** cv_image_ptr = std::any_cast<cv::Mat*>(&backing_handle)) {
			cv::Mat* cv_image = *cv_image_ptr;

			std::vector<cv::Mat> channels;
			cv::split(*cv_image, channels);

			std::vector<cv::Mat> color_channels{ channels[0], channels[1], channels[2] };
			cv::Mat color_only;
			cv::merge(color_channels, color_only);

			cv::Mat as_hsv;
			cv::cvtColor(color_only, as_hsv, cv::COLOR_RGB2HSV);
			as_hsv.forEach<cv::Vec3b>([](cv::Vec3b& pixel, [[maybe_unused]] const int position[]) {
					pixel[1] = static_cast<uchar>(std::min(pixel[1] * 1.2f, 255.0f));
				});
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

	// Add Border
	{
		std::any backing_handle = result.GetBackingHandle();
		if (cv::Mat** cv_image_ptr = std::any_cast<cv::Mat*>(&backing_handle)) {
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
			cv::resize(contours_image, contours_image, cv_image->size());

			cv::Mat alpha_mask;
			cv::merge(std::vector<cv::Mat>{ alpha_channel, alpha_channel, alpha_channel, alpha_channel }, alpha_mask);
			cv::multiply(cv::Scalar::all(1.0f / 255.0f), alpha_mask, alpha_mask);
			cv::multiply(cv::Scalar::all(1.0f) - alpha_mask, contours_image, contours_image);
			
			cv::add(contours_image, *cv_image, *cv_image);
		}
	}

	// Resize to final size
	result.Resize(target_size);
	return result;
};