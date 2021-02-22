#pragma once

#include "decode_audio_file.h"

#include <filesystem>

bool IsSupportedAudioFile(const std::filesystem::path& file_path);
bool HasCachedAudioFile(const std::filesystem::path& file_path, const std::filesystem::path& output_path);
void DeleteCachedAudioFile(const std::filesystem::path& file_path, const std::filesystem::path& output_path);
bool CacheAudioFile(const std::filesystem::path& file_path, const std::filesystem::path& output_path, bool force);
DecodedAudioBuffer LoadCachedAudioFile(const std::filesystem::path& file_path);
