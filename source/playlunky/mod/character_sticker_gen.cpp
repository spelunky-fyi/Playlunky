#include "character_sticker_gen.h"

#include "png_dds_conversion.h"
#include "virtual_filesystem.h"
#include "util/algorithms.h"
#include "util/image.h"

#include <fmt/format.h>
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

	Image sticker_image;
	sticker_image.LoadFromPng(source_sticker);

	Image journal_image;
	journal_image.LoadFromPng(source_journal);

	for (std::string_view character_color : mModdedCharacters) {
		const std::string character_path = fmt::format("Data/Textures/char_{}.png", character_color);
		if (const auto& char_file = vfs.GetFilePath(character_path)) {
			if (const auto* info = algo::find_if(mInfos, [character_color](const CharacterInfo& info) { return info.Color == character_color; })) {
				Image modded_stickers_image;
				modded_stickers_image.LoadFromPng(char_file.value().parent_path() / sticker_file.filename());

				Image modded_journal_image;
				modded_journal_image.LoadFromPng(char_file.value().parent_path() / journal_file.filename());

				std::optional<Image> char_image;
				if (modded_stickers_image.IsEmpty() || modded_journal_image.IsEmpty()) {
					char_image.emplace();
					char_image->LoadFromPng(char_file.value());
				}

				{
					std::optional<Image> sticker_tile;
					if (!modded_stickers_image.IsEmpty()) {
						sticker_tile = modded_stickers_image.GetSubImage(ImageTiling{ s_StickerTileSize, s_StickerTileSize },
							ImageSubRegion{ info->TileIndex.x, info->TileIndex.y, 1, 1 });
					}
					else {
						sticker_tile = char_image->GetSubImage(ImageSubRegion{
						   s_CharacterStickerIndex.x * s_CharacterTileSize, s_CharacterStickerIndex.y * s_CharacterTileSize,
						   s_StickerTileSize, s_StickerTileSize });
					}

					if (sticker_tile == std::nullopt || sticker_tile->IsEmpty()) {
						Image standing_tile = char_image->GetSubImage(ImageTiling{ s_CharacterTileSize, s_CharacterTileSize },
							ImageSubRegion{ 0, 0, 1, 1 });
						standing_tile.Resize(ImageSize{ s_StickerTileSize, s_StickerTileSize });
						sticker_tile = std::move(standing_tile);
					}

					sticker_image.Blit(sticker_tile.value(), ImageTiling{ s_StickerTileSize, s_StickerTileSize }, ImageSubRegion{ info->TileIndex.x, info->TileIndex.y, 1, 1 });
				}

				{
					std::optional<Image> entry_tile;
					if (!modded_journal_image.IsEmpty()) {
						entry_tile = modded_journal_image.GetSubImage(ImageTiling{ s_PeopleTileSize, s_PeopleTileSize },
							ImageSubRegion{ info->TileIndex.x, info->TileIndex.y, 1, 1 });
					}
					else {
						entry_tile = char_image->GetSubImage(ImageSubRegion{
							s_CharacterStickerIndex.x * s_CharacterTileSize, s_CharacterStickerIndex.y * s_CharacterTileSize,
							s_PeopleTileSize, s_PeopleTileSize });
					}

					if (entry_tile == std::nullopt || entry_tile->IsEmpty()) {
						Image standing_tile = char_image->GetSubImage(ImageTiling{ s_CharacterTileSize, s_CharacterTileSize },
							ImageSubRegion{ 0, 0, 1, 1 });
						standing_tile.Resize(ImageSize{ s_PeopleTileSize, s_PeopleTileSize });
						entry_tile = std::move(standing_tile);
					}

					journal_image.Blit(entry_tile.value(), ImageTiling{ s_PeopleTileSize, s_PeopleTileSize }, ImageSubRegion{ info->TileIndex.x, info->TileIndex.y, 1, 1 });
				}
			}
		}
	}

	const auto destination_sticker_file = std::filesystem::path{ destination_folder / sticker_file }.replace_extension(".DDS");
	bool wrote_sticker_file = ConvertRBGAToDds(sticker_image.GetData(), sticker_image.GetWidth(), sticker_image.GetHeight(), destination_sticker_file);

	const auto destination_journal_file = std::filesystem::path{ destination_folder / journal_file }.replace_extension(".DDS");
	bool wrote_journal_file = ConvertRBGAToDds(journal_image.GetData(), journal_image.GetWidth(), journal_image.GetHeight(), destination_journal_file);

	return wrote_sticker_file && wrote_journal_file;
}
