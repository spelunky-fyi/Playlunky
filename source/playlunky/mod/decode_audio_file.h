#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

enum class SoundFormat {
	PCM_8,
	PCM_16,
	PCM_24,
	PCM_32,
	PCM_64,
	PCM_FLOAT,
	PCM_DOUBLE
};

struct DecodedAudioBuffer {
	std::int32_t NumChannels;
	std::int32_t Frequency;
	SoundFormat Format;
	std::unique_ptr<std::byte[]> Data;
	std::size_t DataSize;
};

DecodedAudioBuffer DecodeAudioFile(const std::filesystem::path& file_path);
