#include "character_sticker_gen.h"

#include "png_dds_conversion.h"
#include "virtual_filesystem.h"
#include "util/algorithms.h"
#include "util/image.h"

#include <fmt/format.h>
#include <span>

static constexpr struct { std::uint32_t x, y; } s_CharacterStickerIndex{ .x{ 8 }, .y{ 14 } };
static constexpr struct { std::uint32_t x, y; } s_CharacterJournalIndex{ .x{ 9 }, .y{ 14 } };

static constexpr struct { std::uint32_t x, y; } s_NumCharacterTiles{ .x{ 16 }, .y{ 16 } };
static constexpr struct { std::uint32_t x, y; } s_NumStickerTiles{ .x{ 10 }, .y{ 10 } };
static constexpr struct { std::uint32_t x, y; } s_NumJournalTiles{ .x{ 10 }, .y{ 5 } };

static constexpr std::uint32_t s_DefaultStickerTileSize{ 80 };
static constexpr std::uint32_t s_DefaultJournalTileSize{ 160 };

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

	const auto source_sticker = vfs.GetFilePath(sticker_file).value_or(source_folder / sticker_file);
	const auto source_journal = vfs.GetFilePath(journal_file).value_or(source_folder / journal_file);

	Image sticker_image;
	sticker_image.LoadFromPng(source_sticker);
	const TileDimensions sticker_tile_size{
		.x{ sticker_image.GetWidth() / s_NumStickerTiles.x },
		.y{ sticker_image.GetHeight() / s_NumStickerTiles.y }
	};

	Image journal_image;
	journal_image.LoadFromPng(source_journal);
	const TileDimensions journal_tile_size{
		.x{ journal_image.GetWidth() / s_NumJournalTiles.x },
		.y{ journal_image.GetHeight() / s_NumJournalTiles.y }
	};

	for (std::string_view character_color : mModdedCharacters) {
		const std::string character_path = fmt::format("Data/Textures/char_{}.png", character_color);
		if (const auto& char_file = vfs.GetFilePath(character_path)) {
			if (const auto* info = algo::find_if(mInfos, [character_color](const CharacterInfo& info) { return info.Color == character_color; })) {
				Image modded_stickers_image;
				modded_stickers_image.LoadFromPng(char_file.value().parent_path() / sticker_file.filename());
				const TileDimensions modded_sticker_tile_size{
					.x{ modded_stickers_image.GetWidth() / s_NumStickerTiles.x },
					.y{ modded_stickers_image.GetHeight() / s_NumStickerTiles.y }
				};

				Image modded_journal_image;
				modded_journal_image.LoadFromPng(char_file.value().parent_path() / journal_file.filename());
				const TileDimensions modded_journal_tile_size{
					.x{ modded_journal_image.GetWidth() / s_NumJournalTiles.x },
					.y{ modded_journal_image.GetHeight() / s_NumJournalTiles.y }
				};

				std::optional<Image> char_image;
				TileDimensions char_tile_size{};
				if (modded_stickers_image.IsEmpty() || modded_journal_image.IsEmpty()) {
					char_image.emplace();
					char_image->LoadFromPng(char_file.value());
					char_tile_size = TileDimensions{
						.x{ char_image->GetWidth() / s_NumCharacterTiles.x },
						.y{ char_image->GetHeight() / s_NumCharacterTiles.y }
					};
				}

				{
					std::optional<Image> sticker_tile;
					if (!modded_stickers_image.IsEmpty()) {
						sticker_tile = modded_stickers_image.GetSubImage(
							ImageTiling{ modded_sticker_tile_size },
							ImageSubRegion{ info->TileIndex.x, info->TileIndex.y, 1, 1 });
					}
					else {
						sticker_tile = char_image->GetSubImage(
							ImageTiling{ char_tile_size },
							ImageSubRegion{ s_CharacterStickerIndex.x, s_CharacterStickerIndex.y, 1, 1 });
					}

					if (sticker_tile == std::nullopt || sticker_tile->IsEmpty()) {
						Image standing_tile = char_image->GetSubImage(
							ImageTiling{ char_tile_size },
							ImageSubRegion{ 0, 0, 1, 1 });
						sticker_tile = std::move(standing_tile);
					}

					sticker_tile->Resize(ImageSize{ .x{ sticker_tile_size.x }, .y{ sticker_tile_size.y } });

					sticker_image.Blit(sticker_tile.value(),
						ImageTiling{ sticker_tile_size },
						ImageSubRegion{ info->TileIndex.x, info->TileIndex.y, 1, 1 });
				}

				{
					std::optional<Image> entry_tile;
					if (!modded_journal_image.IsEmpty()) {
						entry_tile = modded_journal_image.GetSubImage(
							ImageTiling{ modded_journal_tile_size },
							ImageSubRegion{ info->TileIndex.x, info->TileIndex.y, 1, 1 });
					}
					else {
						entry_tile = char_image->GetSubImage(
							ImageTiling{ char_tile_size },
							ImageSubRegion{ s_CharacterStickerIndex.x, s_CharacterStickerIndex.y, 2, 2 });
					}

					if (entry_tile == std::nullopt || entry_tile->IsEmpty()) {
						Image standing_tile = char_image->GetSubImage(
							ImageTiling{ char_tile_size },
							ImageSubRegion{ 0, 0, 1, 1 });
						entry_tile = std::move(standing_tile);
					}

					entry_tile->Resize(ImageSize{ .x{ journal_tile_size.x }, .y{ journal_tile_size.y } });

					journal_image.Blit(entry_tile.value(),
						ImageTiling{ journal_tile_size },
						ImageSubRegion{ info->TileIndex.x, info->TileIndex.y, 1, 1 });
				}
			}
		}
	}

	namespace fs = std::filesystem;

	const auto destination_sticker_file = fs::path{ destination_folder / sticker_file }.replace_extension(".DDS");
	bool wrote_sticker_file = ConvertRBGAToDds(sticker_image.GetData(), sticker_image.GetWidth(), sticker_image.GetHeight(), destination_sticker_file);

	const auto destination_journal_file = fs::path{ destination_folder / journal_file }.replace_extension(".DDS");
	bool wrote_journal_file = ConvertRBGAToDds(journal_image.GetData(), journal_image.GetWidth(), journal_image.GetHeight(), destination_journal_file);

	return wrote_sticker_file && wrote_journal_file;
}
