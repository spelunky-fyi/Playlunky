#pragma once

#include <cstdint>
#include <filesystem>
#include <span>

bool IsSupportedFileType(const std::filesystem::path& extension);

bool ConvertRBGAToDds(std::span<const std::uint8_t> source, std::uint32_t width, std::uint32_t height, const std::filesystem::path& destination);
bool ConvertImageToDds(const std::filesystem::path& source, const std::filesystem::path& destination);
bool ConvertDdsToPng(std::span<const std::uint8_t> source, const std::filesystem::path& destination);
bool ConvertDdsToPng(const std::filesystem::path& source, const std::filesystem::path& destination);
