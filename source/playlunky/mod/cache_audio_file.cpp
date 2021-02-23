#include "cache_audio_file.h"

#include "decode_audio_file.h"

#include <cassert>
#include <fstream>

#pragma warning(push, 0)
#include <libnyquist/Decoders.h>
#pragma warning(pop)

inline auto GetCachedAudioFilePath(const std::filesystem::path& file_path, const std::filesystem::path& output_path) {
	return output_path / "raw_audio" / file_path.filename().replace_extension(".raw");
}

bool IsSupportedAudioFile(const std::filesystem::path& file_path) {
	static nqr::NyquistIO s_Loader;
	return s_Loader.IsFileSupported(file_path.extension().string());
}
bool HasCachedAudioFile(const std::filesystem::path& file_path, const std::filesystem::path& output_path) {
	namespace fs = std::filesystem;
	const fs::path output_file_path = GetCachedAudioFilePath(file_path, output_path);
	return fs::exists(output_file_path);
}
void DeleteCachedAudioFile(const std::filesystem::path& file_path, const std::filesystem::path& output_path) {
	namespace fs = std::filesystem;
	const fs::path output_file_path = GetCachedAudioFilePath(file_path, output_path);
	if (fs::exists(output_file_path)) {
		fs::remove(output_file_path);
	}
}
bool CacheAudioFile(const std::filesystem::path& file_path, const std::filesystem::path& output_path, bool force) {
	namespace fs = std::filesystem;
	const fs::path output_file_path = GetCachedAudioFilePath(file_path, output_path);

	if (force || !fs::exists(output_file_path)) {
		DecodedAudioBuffer buffer = DecodeAudioFile(file_path);

		{
			const auto parent_path = output_file_path.parent_path();
			if (!fs::exists(parent_path) && !fs::create_directories(parent_path)) {
				return false;
			}
		}

		if (buffer.DataSize > 0) {
			std::ofstream output_file(output_file_path, std::ios::binary);
			output_file.write(reinterpret_cast<const char*>(&buffer.NumChannels), sizeof(buffer.NumChannels));
			output_file.write(reinterpret_cast<const char*>(&buffer.Frequency), sizeof(buffer.Frequency));
			output_file.write(reinterpret_cast<const char*>(&buffer.Format), sizeof(buffer.Format));
			output_file.write(reinterpret_cast<const char*>(&buffer.DataSize), sizeof(buffer.DataSize));
			output_file.write(reinterpret_cast<const char*>(buffer.Data.get()), buffer.DataSize);
		}
		else {
			return false;
		}
	}

	return true;
}
DecodedAudioBuffer LoadCachedAudioFile(const std::filesystem::path& file_path) {
	DecodedAudioBuffer buffer{};

	if (std::ifstream input_file = std::ifstream(file_path, std::ios::binary)) {
		input_file.read(reinterpret_cast<char*>(&buffer.NumChannels), sizeof(buffer.NumChannels));
		input_file.read(reinterpret_cast<char*>(&buffer.Frequency), sizeof(buffer.Frequency));
		input_file.read(reinterpret_cast<char*>(&buffer.Format), sizeof(buffer.Format));
		input_file.read(reinterpret_cast<char*>(&buffer.DataSize), sizeof(buffer.DataSize));
		buffer.Data = std::make_unique<std::byte[]>(buffer.DataSize);
		input_file.read(reinterpret_cast<char*>(buffer.Data.get()), buffer.DataSize);
	}

	return buffer;
}
