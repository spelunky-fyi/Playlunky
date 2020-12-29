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

bool CharacterStickerGenerator::GenerateStickers(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder,
	const std::filesystem::path& sticker_file, const std::filesystem::path& journal_file, VirtualFilesystem& vfs) {

	std::filesystem::path source_sticker = vfs.GetFilePath(sticker_file).value_or(source_folder / sticker_file);
	std::filesystem::path source_journal = vfs.GetFilePath(journal_file).value_or(source_folder / journal_file);

	std::vector<std::uint8_t> source_sticker_image_buffer;
	std::uint32_t source_sticker_width;
	std::uint32_t source_sticker_height;
	{
		const std::uint32_t error = lodepng::decode(source_sticker_image_buffer, source_sticker_width, source_sticker_height, source_sticker.string(), LCT_RGBA, 8);
		if (error != 0) {
			return false;
		}
	}

	std::vector<std::uint8_t> source_journal_image_buffer;
	std::uint32_t source_journal_width;
	std::uint32_t source_journal_height;
	{
		const std::uint32_t error = lodepng::decode(source_journal_image_buffer, source_journal_width, source_journal_height, source_journal.string(), LCT_RGBA, 8);
		if (error != 0) {
			return false;
		}
	}

	cv::Mat source_sticker_image(static_cast<int>(source_sticker_height), static_cast<int>(source_sticker_width), CV_8UC4, reinterpret_cast<int*>(source_sticker_image_buffer.data()));
	cv::Mat source_journal_image(static_cast<int>(source_journal_height), static_cast<int>(source_journal_width), CV_8UC4, reinterpret_cast<int*>(source_journal_image_buffer.data()));

	for (std::string_view character_color : mModdedCharacters) {
		const std::string character_path = fmt::format("Data/Textures/char_{}.png", character_color);
		if (const auto& char_file = vfs.GetFilePath(character_path)) {
			if (const auto* info = algo::find_if(mInfos, [character_color](const CharacterInfo& info) { return info.Color == character_color; })) {
				std::vector<std::uint8_t> char_image_buffer;
				std::uint32_t char_width;
				std::uint32_t char_height;
				{
					const std::uint32_t error = lodepng::decode(char_image_buffer, char_width, char_height, char_file.value().string(), LCT_RGBA, 8);
					if (error != 0) {
						return false;
					}
				}
				
				cv::Mat char_image(static_cast<int>(char_height), static_cast<int>(char_width), CV_8UC4, reinterpret_cast<int*>(char_image_buffer.data()));

				{
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
					sticker_tile.copyTo(source_sticker_image(cv::Rect(tile_x, tile_y, s_StickerTileSize, s_StickerTileSize)));
				}

				{
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
					entry_tile.copyTo(source_journal_image(cv::Rect(tile_x, tile_y, s_PeopleTileSize, s_PeopleTileSize)));
				}
			}
		}
	}

	const auto destination_sticker_file = std::filesystem::path{ destination_folder / sticker_file }.replace_extension(".DDS");
	bool wrote_sticker_file = ConvertRBGAToDds(source_sticker_image_buffer, source_sticker_width, source_sticker_height, destination_sticker_file);

	const auto destination_journal_file = std::filesystem::path{ destination_folder / journal_file }.replace_extension(".DDS");
	bool wrote_journal_file = ConvertRBGAToDds(source_journal_image_buffer, source_journal_width, source_journal_height, destination_journal_file);

	return wrote_sticker_file && wrote_journal_file;
}
