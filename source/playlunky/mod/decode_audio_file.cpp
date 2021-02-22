#include "decode_audio_file.h"

#include "log.h"
#include "util/algorithms.h"
#include "util/on_scope_exit.h"

#include <cassert>

DecodedAudioBuffer DecodeAudioFile(const std::filesystem::path& file_path, AudioFileType type)
{
	if (type == AudioFileType::Guess) {
		const auto ext = file_path.extension();
		if (ext == ".wav") {
			type = AudioFileType::Wav;
		}
		else if (ext == ".ogg") {
			type = AudioFileType::Ogg;
		}
		else if (ext == ".mp3") {
			type = AudioFileType::Mp3;
		}
		else {
			LogInfo("Can not guess audio type from extension {}, this file won't be decoded...", ext.string());
		}
	}

	switch (type) {
	case AudioFileType::Wav:
		return DecodeWavFile(file_path);
	case AudioFileType::Ogg:
		return DecodeOggFile(file_path);
	case AudioFileType::Mp3:
		return DecodeMp3File(file_path);
	default:
		return {};
	}
}

DecodedAudioBuffer DecodeWavFile(const std::filesystem::path& file_path)
{
	FILE* file{ nullptr };
	auto error = _wfopen_s(&file, file_path.c_str(), L"rb");
	if (error == 0 && file != nullptr) {
		auto close_file = OnScopeExit{ [file]() { fclose(file); } };

		{ // RIFF-WAVE chunk
			char riff_sign[4]{};
			std::fread(riff_sign, 1, 4, file);
			assert(algo::case_insensitive_equal(riff_sign, "RIFF"));
			std::uint32_t chunk_size{};
			std::fread(&chunk_size, 4, 1, file);
			char wave_sign[4]{};
			std::fread(wave_sign, 1, 4, file);
			assert(algo::case_insensitive_equal(wave_sign, "WAVE"));
		}

		DecodedAudioBuffer decoded_buffer{};

		{
			// FMT chunk
			char fmt_sign[4]{};
			std::fread(fmt_sign, 1, 4, file);
			assert(algo::case_insensitive_equal(fmt_sign, "FMT "));
			std::uint32_t chunk_size{};
			std::fread(&chunk_size, 4, 1, file);

			struct WAVEFORMAT {
				std::uint16_t Format;
				std::uint16_t NumChannels;
				std::uint32_t SamplesPerSecond;
				std::uint32_t AverageBytesPerSecond;
				std::uint16_t BlockAlignment;
				std::uint16_t BitsPerSample;
			};
			WAVEFORMAT format{};
			assert(chunk_size >= sizeof(WAVEFORMAT));
			std::fread(&format, sizeof(WAVEFORMAT), 1, file);
			if (chunk_size > sizeof(WAVEFORMAT)) {
				std::fseek(file, chunk_size - sizeof(WAVEFORMAT), SEEK_CUR);
			}

			decoded_buffer.NumChannels = format.NumChannels;
			decoded_buffer.Frequency = format.SamplesPerSecond;
			decoded_buffer.BitsPerSample = format.BitsPerSample;
		}

		{ // data chunk
			char data_sign[4]{};
			std::fread(data_sign, 1, 4, file);
			std::uint32_t chunk_size{};
			std::fread(&chunk_size, 4, 1, file);
			while (!algo::case_insensitive_equal(data_sign, "data")) {
				std::fseek(file, chunk_size, SEEK_CUR);
				std::fread(data_sign, 1, 4, file);
				std::fread(&chunk_size, 4, 1, file);
			}
			decoded_buffer.Data = std::make_unique<std::byte[]>(chunk_size);
			std::fread(decoded_buffer.Data.get(), 1, chunk_size, file);
			decoded_buffer.DataSize = chunk_size;
		}

		return decoded_buffer;
	}

	return {};
}
DecodedAudioBuffer DecodeOggFile(const std::filesystem::path& /*file_path*/)
{
	return {};
}
DecodedAudioBuffer DecodeMp3File(const std::filesystem::path& /*file_path*/)
{
	return {};
}
