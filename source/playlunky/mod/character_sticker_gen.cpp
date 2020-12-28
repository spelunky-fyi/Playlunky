#include "character_sticker_gen.h"

#include "png_dds_conversion.h"
#include "virtual_filesystem.h"
#include "util/algorithms.h"

#pragma warning(push)
#pragma warning(disable : 5054)
#include <opencv2/imgproc.hpp>
#pragma warning(pop)

#include <fmt/format.h>
#include <lodepng.h>
#include <span>

static constexpr struct { std::uint32_t x, y; } s_CharacterStickerIndex{ .x{ 8 }, .y{ 14 } };
static constexpr struct { std::uint32_t x, y; } s_CharacterJournalIndex{ .x{ 9 }, .y{ 14 } };

static constexpr std::uint32_t s_CharacterTileSize = 128;
static constexpr std::uint32_t s_StickerTileSize = 80;
static constexpr std::uint32_t s_PeopleTileSize = 160;

bool CharacterStickerGenerator::RegisterCharacter(std::string_view character_color, bool outdated) {
	if (const auto* info = algo::find_if(mInfos, [character_color](const CharacterInfo& info) { return info.Color == character_color; })) {
		mNeedsRegen = mNeedsRegen || outdated;

		if (!algo::contains(mModdedCharacters, info->Color)) {
			mModdedCharacters.push_back(info->Color);
		}

		return true;
	}

	return false;
}

bool CharacterStickerGenerator::GenerateStickers(const std::filesystem::path& source, const std::filesystem::path& destination, VirtualFilesystem& vfs) {
	std::vector<std::uint8_t> source_image_buffer;
	std::uint32_t source_width;
	std::uint32_t source_height;
	std::uint32_t error = lodepng::decode(source_image_buffer, source_width, source_height, source.string(), LCT_RGBA, 8);
	if (error != 0) {
		return false;
	}

	cv::Mat source_image(static_cast<int>(source_height), static_cast<int>(source_width), CV_8UC4, reinterpret_cast<int*>(source_image_buffer.data()));

	for (std::string_view character_color : mModdedCharacters) {
		const std::string character_path = fmt::format("Data/Textures/char_{}.png", character_color);
		if (const auto& char_file = vfs.GetFilePath(character_path)) {
			if (const auto* info = algo::find_if(mInfos, [character_color](const CharacterInfo& info) { return info.Color == character_color; })) {
				std::vector<std::uint8_t> char_image_buffer;
				std::uint32_t char_width;
				std::uint32_t char_height;
				error = lodepng::decode(char_image_buffer, char_width, char_height, char_file.value().string(), LCT_RGBA, 8);
				if (error != 0) {
					return false;
				}

				cv::Mat char_image(static_cast<int>(char_height), static_cast<int>(char_width), CV_8UC4, reinterpret_cast<int*>(char_image_buffer.data()));

				static constexpr auto sticker_tile_x = s_CharacterStickerIndex.x * s_CharacterTileSize;
				static constexpr auto sticker_tile_y = s_CharacterStickerIndex.y * s_CharacterTileSize;
				cv::Mat sticker_tile = char_image(cv::Rect(sticker_tile_x, sticker_tile_y, s_StickerTileSize, s_StickerTileSize));

				if (cv::mean(sticker_tile) == cv::Scalar{ 0 }) {
					cv::Mat standing_tile = char_image(cv::Rect(0, 0, s_CharacterTileSize, s_CharacterTileSize));

					cv::Mat standing_tile_resized;
					cv::resize(standing_tile, standing_tile_resized, cv::Size(s_StickerTileSize, s_StickerTileSize));

					sticker_tile = std::move(standing_tile_resized);
				}

				const auto tile_x = info->TileIndex.x * s_StickerTileSize;
				const auto tile_y = info->TileIndex.y * s_StickerTileSize;
				sticker_tile.copyTo(source_image(cv::Rect(tile_x, tile_y, s_StickerTileSize, s_StickerTileSize)));
			}
		}
	}

	return ConvertRBGAToDds(source_image_buffer, source_width, source_height, destination);
}

bool CharacterStickerGenerator::GenerateJournal(const std::filesystem::path& source, const std::filesystem::path& destination, VirtualFilesystem& vfs) {
	std::vector<std::uint8_t> source_image_buffer;
	std::uint32_t source_width;
	std::uint32_t source_height;
	std::uint32_t error = lodepng::decode(source_image_buffer, source_width, source_height, source.string(), LCT_RGBA, 8);
	if (error != 0) {
		return false;
	}

	cv::Mat source_image(static_cast<int>(source_height), static_cast<int>(source_width), CV_8UC4, reinterpret_cast<int*>(source_image_buffer.data()));

	static constexpr std::uint32_t people_tile_size = 160;

	for (std::string_view character_color : mModdedCharacters) {
		const std::string character_path = fmt::format("Data/Textures/char_{}.png", character_color);
		if (const auto& char_file = vfs.GetFilePath(character_path)) {
			if (const auto* info = algo::find_if(mInfos, [character_color](const CharacterInfo& info) { return info.Color == character_color; })) {
				std::vector<std::uint8_t> char_image_buffer;
				std::uint32_t char_width;
				std::uint32_t char_height;
				error = lodepng::decode(char_image_buffer, char_width, char_height, char_file.value().string(), LCT_RGBA, 8);
				if (error != 0) {
					return false;
				}

				cv::Mat char_image(static_cast<int>(char_height), static_cast<int>(char_width), CV_8UC4, reinterpret_cast<int*>(char_image_buffer.data()));

				static constexpr auto entry_tile_x = s_CharacterJournalIndex.x * s_CharacterTileSize;
				static constexpr auto entry_tile_y = s_CharacterJournalIndex.y * s_CharacterTileSize;
				cv::Mat entry_tile = char_image(cv::Rect(entry_tile_x, entry_tile_y, s_PeopleTileSize, s_PeopleTileSize));

				if (cv::mean(entry_tile) == cv::Scalar{ 0 }) {
					cv::Mat standing_tile = char_image(cv::Rect(0, 0, s_CharacterTileSize, s_CharacterTileSize));

					cv::Mat standing_tile_resized;
					cv::resize(standing_tile, standing_tile_resized, cv::Size(s_PeopleTileSize, s_PeopleTileSize));

					entry_tile = std::move(standing_tile_resized);
				}

				const auto tile_x = info->TileIndex.x * s_PeopleTileSize;
				const auto tile_y = info->TileIndex.y * s_PeopleTileSize;
				entry_tile.copyTo(source_image(cv::Rect(tile_x, tile_y, s_PeopleTileSize, s_PeopleTileSize)));
			}
		}
	}


	return ConvertRBGAToDds(source_image_buffer, source_width, source_height, destination);
}
