#pragma once

#include <cstdint>
#include <filesystem>
#include <span>

bool ConvertPngToDds(const std::filesystem::path& source, const std::filesystem::path& destination);
bool ConvertDdsToPng(std::span<const std::uint8_t> source, const std::filesystem::path& destination);
