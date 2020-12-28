#pragma once

#include <cstdint>
#include <filesystem>
#include <span>

bool ConvertRBGAToDds(std::span<const std::uint8_t> source, std::uint32_t width, std::uint32_t height, const std::filesystem::path& destination);
bool ConvertPngToDds(const std::filesystem::path& source, const std::filesystem::path& destination);
bool ConvertDdsToPng(std::span<const std::uint8_t> source, const std::filesystem::path& destination);
bool ConvertDdsToPng(const std::filesystem::path& source, const std::filesystem::path& destination);
