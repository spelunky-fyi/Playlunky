#include "decode_audio_file.h"

#include "log.h"

#include <cassert>

#pragma warning(push, 0)
#include <libnyquist/Decoders.h>
#pragma warning(pop)

DecodedAudioBuffer DecodeAudioFile(const std::filesystem::path& file_path)
{
#ifdef _DEBUG
	if (file_path.extension() == ".ogg") {
		return {};
	}
#endif

	nqr::AudioData decoded_data;
	nqr::NyquistIO loader;
	loader.Load(&decoded_data, file_path.string());

	const auto data_size = decoded_data.samples.size() * (GetFormatBitsPerSample(decoded_data.sourceFormat) / 8);
	auto data = std::make_unique<std::byte[]>(data_size);
	if (decoded_data.sourceFormat == nqr::PCM_FLT) {
		memcpy(data.get(), decoded_data.samples.data(), data_size);
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
