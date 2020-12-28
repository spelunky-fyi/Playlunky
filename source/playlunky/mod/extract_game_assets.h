#pragma once

#include <array>
#include <filesystem>
#include <span>

bool ExtractGameAssets(std::span<const std::filesystem::path> files, const std::filesystem::path& destination);

template<std::size_t N>
bool ExtractGameAssets(const std::array<std::filesystem::path, N>& files, const std::filesystem::path& destination) {
	return ExtractGameAssets(std::span{ files.data(), N }, destination);
}
