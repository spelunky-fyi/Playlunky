#include "sprite_sheet_merger.h"

#include "entity_data_extraction.h"
#include "generate_sticker_pixel_art.h"
#include "log.h"
#include "png_dds_conversion.h"
#include "virtual_filesystem.h"
#include "playlunky_settings.h"
#include "util/algorithms.h"
#include "util/format.h"

#include <cassert>

#pragma warning(push)
#pragma warning(disable : 5054)
#pragma warning(disable : 4996)
#pragma warning(disable : 4244)
#include <opencv2/imgproc.hpp>
#pragma warning(pop)

SpriteSheetMerger::SpriteSheetMerger(const PlaylunkySettings& settings)
	: mRandomCharacterSelectEnabled{ settings.GetBool("settings", "random_character_select", false) || settings.GetBool("sprite_settings", "random_character_select", false) }
	, mGenerateCharacterJournalStickersEnabled{ settings.GetBool("sprite_settings", "generate_character_journal_stickers", true) }
	, mGenerateCharacterJournalEntriesEnabled{ settings.GetBool("sprite_settings", "generate_character_journal_entries", true) }
	, mGenerateStickerPixelArtEnabled{ settings.GetBool("sprite_settings", "generate_sticker_pixel_art", true) }
{}
SpriteSheetMerger::~SpriteSheetMerger() = default;

void SpriteSheetMerger::GatherSheetData() {
	m_EntityDataExtractor = std::make_unique<EntityDataExtractor>();
	m_EntityDataExtractor->PreloadEntityMappings();

	MakeItemsSheet();
	MakeJournalItemsSheet();
	MakeJournalMonstersSheet();
	MakeJournalMonstersBigSheet();
	MakeJournalPeopleSheet();
	MakeJournalStickerSheet();
	MakeMountsTargetSheet();
	MakePetsTargetSheet();
	MakeMonstersTargetSheet();
	MakeBigMonstersTargetSheet();
	MakeCharacterTargetSheet("black");
	MakeCharacterTargetSheet("blue");
	MakeCharacterTargetSheet("cerulean");
	MakeCharacterTargetSheet("cinnabar");
	MakeCharacterTargetSheet("cyan");
	MakeCharacterTargetSheet("eggchild");
	MakeCharacterTargetSheet("gold");
	MakeCharacterTargetSheet("gray");
	MakeCharacterTargetSheet("green");
	MakeCharacterTargetSheet("hired");
	MakeCharacterTargetSheet("iris");
	MakeCharacterTargetSheet("khaki");
	MakeCharacterTargetSheet("lemon");
	MakeCharacterTargetSheet("lime");
	MakeCharacterTargetSheet("magenta");
	MakeCharacterTargetSheet("olive");
	MakeCharacterTargetSheet("orange");
	MakeCharacterTargetSheet("pink");
	MakeCharacterTargetSheet("red");
	MakeCharacterTargetSheet("violet");
	MakeCharacterTargetSheet("white");
	MakeCharacterTargetSheet("yellow");
	MakeMenuLeaderTargetSheet();

	m_EntityDataExtractor = nullptr;
}

void SpriteSheetMerger::RegisterSheet(const std::filesystem::path& full_sheet, bool outdated, bool deleted) {
	if (RegisteredSourceSheet* registered_sheet = algo::find_if(m_RegisteredSourceSheets,
		[&full_sheet](const RegisteredSourceSheet& sheet) { return sheet.Path == full_sheet; })) {
		registered_sheet->Outdated = registered_sheet->Outdated || outdated;
		registered_sheet->Deleted = registered_sheet->Deleted || deleted;
		return;
	}
	m_RegisteredSourceSheets.push_back(RegisteredSourceSheet{
			.Path = full_sheet,
			.Outdated = outdated,
			.Deleted = deleted
		});
}

bool SpriteSheetMerger::NeedsRegeneration(const std::filesystem::path& destination_folder) const {
	for (const TargetSheet& target_sheet : m_TargetSheets) {
		if (NeedsRegen(target_sheet, destination_folder)) {
			return true;
		}
	}
	return false;
}

bool SpriteSheetMerger::NeedsRegen(const TargetSheet& target_sheet, const std::filesystem::path& destination_folder) const {
	namespace fs = std::filesystem;

	const bool does_exist = fs::exists(fs::path{ destination_folder / target_sheet.Path }.replace_extension(".DDS"));
	const bool random_select = target_sheet.RandomSelect;
	for (const SourceSheet& source_sheet : target_sheet.SourceSheets) {
		if (const RegisteredSourceSheet* registered_sheet = algo::find_if(m_RegisteredSourceSheets,
			[&source_sheet](const RegisteredSourceSheet& sheet) { return sheet.Path == source_sheet.Path; })) {
			if (!does_exist
				|| random_select
				|| registered_sheet->Outdated
				|| registered_sheet->Deleted) {
				return true;
			}
		}
	}

	return false;
}

bool SpriteSheetMerger::GenerateRequiredSheets(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder, VirtualFilesystem& vfs) {
	namespace fs = std::filesystem;

	for (const TargetSheet& target_sheet : m_TargetSheets) {
		if (NeedsRegen(target_sheet, destination_folder)) {
			const auto target_file_path = vfs.GetFilePath(target_sheet.Path).value_or(source_folder / target_sheet.Path);
			Image target_image;
			target_image.LoadFromPng(target_file_path);

			// TODO: Scaling based on largest target image?
			const float target_width_scaling = static_cast<float>(target_image.GetWidth()) / target_sheet.Size.Width;
			const float target_height_scaling = static_cast<float>(target_image.GetHeight()) / target_sheet.Size.Height;

			for (const SourceSheet& source_sheet : target_sheet.SourceSheets) {
				const auto source_file_path = [&vfs, &source_sheet, random_select = target_sheet.RandomSelect]() {
					if (!random_select) {
						return vfs.GetFilePath(source_sheet.Path);
					}
					else {
						return vfs.GetRandomFilePath(source_sheet.Path);
					}
				}();
				if (source_file_path) {
					Image source_image;
					source_image.LoadInfoFromPng(source_file_path.value());

					// Skip images with wrong aspect ratio
					const std::uint64_t aspect_ratio_offset = 
						std::abs(
							static_cast<int64_t>(source_sheet.Size.Width) * source_image.GetHeight()
							- static_cast<int64_t>(source_image.GetWidth()) * source_sheet.Size.Height
						);
					// We accept images that are 10 pixels off in width
					static constexpr std::uint64_t s_AcceptedPixelError{ 10 };
					if (aspect_ratio_offset > source_sheet.Size.Height * s_AcceptedPixelError) {
						continue;
					}

					const float source_width_scaling = static_cast<float>(source_image.GetWidth()) / source_sheet.Size.Width;
					const float source_height_scaling = static_cast<float>(source_image.GetHeight()) / source_sheet.Size.Height;

					source_image.LoadFromPng(source_file_path.value());

					for (const TileMapping& tile_mapping : source_sheet.TileMap) {
						const ImageSubRegion source_region = ImageSubRegion{
							.x{ static_cast<std::int32_t>(tile_mapping.SourceTile.Left * source_width_scaling) },
							.y{ static_cast<std::int32_t>(tile_mapping.SourceTile.Top * source_height_scaling) },
							.width{ static_cast<std::uint32_t>((tile_mapping.SourceTile.Right - tile_mapping.SourceTile.Left) * source_width_scaling) },
							.height{ static_cast<std::uint32_t>((tile_mapping.SourceTile.Bottom - tile_mapping.SourceTile.Top) * source_height_scaling) },
						};
						const ImageSubRegion target_region = ImageSubRegion{
							.x{ static_cast<std::int32_t>(tile_mapping.TargetTile.Left * target_height_scaling) },
							.y{ static_cast<std::int32_t>(tile_mapping.TargetTile.Top * source_height_scaling) },
							.width{ static_cast<std::uint32_t>((tile_mapping.TargetTile.Right - tile_mapping.TargetTile.Left) * target_height_scaling) },
							.height{ static_cast<std::uint32_t>((tile_mapping.TargetTile.Bottom - tile_mapping.TargetTile.Top) * source_height_scaling) },
						};

						if (!source_image.ContainsSubRegion(source_region)) {
							LogError("Source image {} does not contain tile ({}, {}, {}, {}), image size is ({}, {})... Tile expected from target image {}...", source_file_path.value().string(),
								source_region.x, source_region.y, source_region.width, source_region.height,
								source_image.GetWidth(), source_image.GetHeight(), target_file_path.string());
							continue;
						}
						if (!target_image.ContainsSubRegion(target_region)) {
							LogError("Target image {} does not contain tile ({}, {}, {}, {}), image size is ({}, {})... Tile expected from source image {}...", target_file_path.string(),
								target_region.x, target_region.y, target_region.width, target_region.height,
								target_image.GetWidth(), target_image.GetHeight(), source_file_path.value().string());
							continue;
						}

						Image source_tile = source_image.GetSubImage(source_region);
						const auto target_size = ::ImageSize{ .x{ static_cast<std::uint32_t>(target_region.width) }, .y{ static_cast<std::uint32_t>(target_region.height) } };

						if (source_sheet.Processing) {
							source_tile = source_sheet.Processing(std::move(source_tile), target_size);
						}

						if (source_tile.GetWidth() != target_size.x || source_tile.GetHeight() != target_size.y) {
							source_tile.Resize(target_size);
						}

						try {
							target_image.Blit(source_tile, target_region);
						}
						catch (cv::Exception& e) {
							fmt::print("{}", e.what());
						}
					}
				}
			}

			const auto destination_file_path = fs::path{ destination_folder / target_sheet.Path }.replace_extension(".DDS");
			if (!ConvertRBGAToDds(target_image.GetData(), target_image.GetWidth(), target_image.GetHeight(), destination_file_path)) {
				return false;
			}
		}
	}

	return true;
}
