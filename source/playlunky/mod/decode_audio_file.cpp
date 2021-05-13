#include "decode_audio_file.h"

#include "log.h"

#include <cassert>

#pragma warning(push, 0)
#include <libnyquist/Decoders.h>
#pragma warning(pop)

//#define SHOW_AUDIO_WAVEFORM_ON_LOAD

#ifdef SHOW_AUDIO_WAVEFORM_ON_LOAD
#include <cmath>
#pragma warning(push)
#pragma warning(disable : 5054)
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#pragma warning(pop)
#endif

DecodedAudioBuffer DecodeAudioFile(const std::filesystem::path& file_path)
{
	nqr::AudioData decoded_data;
	nqr::NyquistIO loader;
	loader.Load(&decoded_data, file_path.string());

	const auto data_size = decoded_data.samples.size() * (GetFormatBitsPerSample(decoded_data.sourceFormat) / 8);
	auto data = std::make_unique<std::byte[]>(data_size);
	if (decoded_data.sourceFormat == nqr::PCM_FLT) {
		memcpy(data.get(), decoded_data.samples.data(), data_size);

#ifdef SHOW_AUDIO_WAVEFORM_ON_LOAD
		if (file_path.extension() == ".ogg") {
			std::vector<int> image_data;
			image_data.resize(720 * 720);
			cv::Mat image{ 720, 720, CV_8UC4, image_data.data() };

			const float max_amplitude = [&]() {
				float max = 0.0;
				for (size_t i = 0; i < decoded_data.samples.size(); i++) {
					max = std::max(std::abs(decoded_data.samples[i]), max);
				}
				return max;
			}();

			size_t prev_x{};
			size_t prev_y{};
			for (size_t i = 0; i < decoded_data.samples.size(); i++) {
				size_t x = i * 720 / decoded_data.samples.size();
				size_t y = static_cast<size_t>((1.0f - std::abs(decoded_data.samples[i]) / max_amplitude) * 719);

				cv::line(image, { (int)prev_x, (int)prev_y }, { (int)x, (int)y }, CV_RGB(255, 0, 0), 1, 4, 0);

				prev_x = x;
				prev_y = y;
			}

			try {
				cv::imshow(file_path.string(), image);
				cv::waitKey();
			}
			catch (cv::Exception& e) {
				fmt::print("{}", e.what());
			}
		}
#endif
	}
	else {
		nqr::ConvertFromFloat32((std::uint8_t*)data.get(), decoded_data.samples.data(), decoded_data.samples.size(), decoded_data.sourceFormat);
	}

	return DecodedAudioBuffer{
		.NumChannels = decoded_data.channelCount,
		.Frequency = decoded_data.sampleRate,
		.Format = static_cast<SoundFormat>(decoded_data.sourceFormat - 1),
		.Data = std::move(data),
		.DataSize = data_size
	};
}
