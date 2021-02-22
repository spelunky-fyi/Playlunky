#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

struct DecodedAudioBuffer {
	std::int32_t NumChannels;
	std::int32_t Frequency;
	std::int32_t BitsPerSample;
	std::unique_ptr<std::byte[]> Data;
	std::size_t DataSize;
};

enum class AudioFileType {
	Guess,
	Wav,
	Ogg,
	Mp3
};
DecodedAudioBuffer DecodeAudioFile(const std::filesystem::path& file_path, AudioFileType type = AudioFileType::Guess);

DecodedAudioBuffer DecodeWavFile(const std::filesystem::path& file_path);
DecodedAudioBuffer DecodeOggFile(const std::filesystem::path& file_path);
DecodedAudioBuffer DecodeMp3File(const std::filesystem::path& file_path);
