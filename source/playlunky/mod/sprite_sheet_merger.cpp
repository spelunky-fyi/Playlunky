#include "sprite_sheet_merger.h"

#include "png_dds_conversion.h"
#include "virtual_filesystem.h"
#include "util/algorithms.h"
#include "util/image.h"

#include <cassert>
#include <fmt/format.h>

#pragma warning(push)
#pragma warning(disable : 5054)
#include <opencv2/imgproc.hpp>
#pragma warning(pop)

SpriteSheetMerger::SpriteSheetMerger()
	: m_TargetSheets{
		MakeJournalPeopleSheet(),
		MakeJournalStickerSheet(),
		MakeMountsTargetSheet(),
		MakePetsTargetSheet(),
		MakeCharacterTargetSheet("black"),
		MakeCharacterTargetSheet("blue"),
		MakeCharacterTargetSheet("cerulean"),
		MakeCharacterTargetSheet("cinnabar"),
		MakeCharacterTargetSheet("cyan"),
		MakeCharacterTargetSheet("eggchild"),
		MakeCharacterTargetSheet("gold"),
		MakeCharacterTargetSheet("gray"),
		MakeCharacterTargetSheet("green"),
		MakeCharacterTargetSheet("hired"),
		MakeCharacterTargetSheet("iris"),
		MakeCharacterTargetSheet("khaki"),
		MakeCharacterTargetSheet("lemon"),
		MakeCharacterTargetSheet("lime"),
		MakeCharacterTargetSheet("magenta"),
		MakeCharacterTargetSheet("olive"),
		MakeCharacterTargetSheet("orange"),
		MakeCharacterTargetSheet("pink"),
		MakeCharacterTargetSheet("red"),
		MakeCharacterTargetSheet("violet"),
		MakeCharacterTargetSheet("white"),
		MakeCharacterTargetSheet("yellow"),
	}
{}

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

	if (!fs::exists(fs::path{ destination_folder / target_sheet.Path }.replace_extension(".DDS"))) {
		return true;
	}
	for (const SourceSheet& source_sheet : target_sheet.SourceSheets) {
		if (const RegisteredSourceSheet* registered_sheet = algo::find_if(m_RegisteredSourceSheets,
			[&source_sheet](const RegisteredSourceSheet& sheet) { return sheet.Path == source_sheet.Path; })) {
			if (registered_sheet->Outdated || registered_sheet->Deleted) {
				return true;
			}
		}
	}

	return false;
}


bool SpriteSheetMerger::GenerateRequiredSheets(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder, VirtualFilesystem& vfs) {
	for (const TargetSheet& target_sheet : m_TargetSheets) {
		if (NeedsRegen(target_sheet, destination_folder)) {
			const auto target_file_path = vfs.GetFilePath(target_sheet.Path).value_or(source_folder / target_sheet.Path);
			Image target_image;
			target_image.LoadFromPng(target_file_path);

			// TODO: Scaling based on largest source image
			// TODO: Scaling based on largest target image
			assert(target_image.GetWidth() == target_sheet.Size.Width && target_image.GetHeight() == target_sheet.Size.Height);
			
			for (const SourceSheet& source_sheet : target_sheet.SourceSheets) {
				const auto source_file_path = vfs.GetFilePath(source_sheet.Path);
				if (source_file_path) {
					Image source_image;
					source_image.LoadFromPng(source_file_path.value());

					// TODO: Scaling based on actual image sizes (in case mods ship with scaled assets)
					assert(source_image.GetWidth() == source_sheet.Size.Width && source_image.GetHeight() == source_sheet.Size.Height);

					for (const TileMapping& tile_mapping : source_sheet.TileMap) {
						const ImageSubRegion source_region = ImageSubRegion{
							.x{ tile_mapping.SourceTile.Left },
							.y{ tile_mapping.SourceTile.Top },
							.width{ tile_mapping.SourceTile.Right - tile_mapping.SourceTile.Left },
							.height{ tile_mapping.SourceTile.Bottom - tile_mapping.SourceTile.Top },
						};
						const ImageSubRegion target_region = ImageSubRegion{
							.x{ tile_mapping.TargetTile.Left },
							.y{ tile_mapping.TargetTile.Top },
							.width{ tile_mapping.TargetTile.Right - tile_mapping.TargetTile.Left },
							.height{ tile_mapping.TargetTile.Bottom - tile_mapping.TargetTile.Top },
						};

						// TODO: Scaling based on subimage sizes
						Image source_tile = source_image.GetSubImage(source_region);
						try {
							target_image.Blit(source_tile, target_region);
						}
						catch (cv::Exception& e) {
							fmt::print("{}", e.what());
						}
					}
				}
			}

			namespace fs = std::filesystem;
			const auto destination_file_path = fs::path{ destination_folder / target_sheet.Path }.replace_extension(".DDS");
			if (!ConvertRBGAToDds(target_image.GetData(), target_image.GetWidth(), target_image.GetHeight(), destination_file_path)) {
				return false;
			}
		}
	}

	return true;
}

// Note: All the `TileMap = std::vector<TileMapping>{ ... }` code is because of a bug in MSVC
SpriteSheetMerger::TargetSheet SpriteSheetMerger::MakeJournalPeopleSheet() {
	std::vector<SourceSheet> source_sheets;

	{
		std::uint32_t char_x{ 0 };
		std::uint32_t char_y{ 0 };
		for (std::string_view color : { "yellow", "magenta", "cyan", "black", "cinnabar", "green", "olive",
										"white", "cerulean", "blue", "lime", "lime", "lemon", "iris", "gold",
										"red", "pink", "violet", "gray", "khaki", "orange" }) {
			source_sheets.push_back(SourceSheet{
					.Path{ fmt::format("Data/Textures/Merged/char_{}_full.png", color) },
					.Size{ .Width{ 2048 }, .Height{ 2160 } },
					.TileMap = std::vector<TileMapping>{
						TileMapping{
							.SourceTile{ 0, 1920, 160, 2080 },
							.TargetTile{ char_x * 160, char_y * 160, char_x * 160 + 160, char_y * 160 + 160 },
						}
					}
				});
			char_x++;
			if (char_x >= 10) {
				char_x = 0;
				char_y++;
			}
		}
	}
	
	source_sheets.push_back(SourceSheet{
			.Path{ "Data/Textures/Merged/char_eggchild_full.png" },
			.Size{ .Width{ 2048 }, .Height{ 2080 } },
			.TileMap = std::vector<TileMapping>{
				TileMapping{
					.SourceTile{ 0, 1920, 160, 2080 },
					.TargetTile{ 160, 320, 320, 480 },
				}
			}
		});
	source_sheets.push_back(SourceSheet{
			.Path{ "Data/Textures/Merged/char_hired_full.png" },
			.Size{ .Width{ 2048 }, .Height{ 2080 } },
			.TileMap = std::vector<TileMapping>{
				TileMapping{
					.SourceTile{ 0, 1920, 160, 2080 },
					.TargetTile{ 160, 320, 320, 480 },
				}
			}
		});

	return TargetSheet{
		.Path{ "Data/Textures/journal_entry_people.png" },
		.Size{.Width{ 1600 }, .Height{ 800 } },
		.SourceSheets{ std::move(source_sheets) }
	};
}
SpriteSheetMerger::TargetSheet SpriteSheetMerger::MakeJournalStickerSheet() {
	std::vector<SourceSheet> source_sheets;

	{
		std::uint32_t char_x{ 0 };
		std::uint32_t char_y{ 0 };
		for (std::string_view color : { "yellow", "magenta", "cyan", "black", "cinnabar", "green", "olive",
			"white", "cerulean", "blue", "lime", "lime", "lemon", "iris", "gold",
			"red", "pink", "violet", "gray", "khaki", "orange" }) {
			source_sheets.push_back(SourceSheet{
					.Path{ fmt::format("Data/Textures/Merged/char_{}_full.png", color) },
					.Size{.Width{ 2048 }, .Height{ 2160 } },
					.TileMap = std::vector<TileMapping>{
						TileMapping{
							.SourceTile{ 0, 2080, 80, 2160 },
							.TargetTile{ char_x * 80, char_y * 80, char_x * 80 + 80, char_y * 80 + 80 },
						}
					}
				});
			char_x++;
			if (char_x >= 10) {
				char_x = 0;
				char_y++;
			}
		}
	}

	return TargetSheet{
		.Path{ "Data/Textures/journal_stickers.png" },
		.Size{.Width{ 800 }, .Height{ 800 } },
		.SourceSheets{ std::move(source_sheets) }
	};
}
SpriteSheetMerger::TargetSheet SpriteSheetMerger::MakeMountsTargetSheet() {
	std::vector<std::pair<std::string_view, std::uint32_t>> name_to_idx{
		{ "turkey", 0 },
		{ "rockdog", 1 },
		{ "axolotl", 2 },
		{ "qilin", 3 }
	};

	std::vector<SourceSheet> source_sheets;
	for (const auto& [pet_name, idx] : name_to_idx) {
		const std::uint32_t image_height = pet_name == "turkey" ? 960 : 672;
		source_sheets.push_back(SourceSheet{
				.Path{ fmt::format("Data/Textures/Merged/{}_full.png", pet_name) },
				.Size{.Width{ 2048 }, .Height{ image_height } },
				.TileMap = std::vector<TileMapping>{
					TileMapping{
						.SourceTile{ 0, 0, 1536, 512 },
						.TargetTile{ 0, 512 * idx, 1536, 512 * idx + 512 },
					}
				}
			});
	}

	return TargetSheet{
		.Path{ "Data/Textures/mounts.png" },
		.Size{.Width{ 2048 }, .Height{ 2048 } },
		.SourceSheets{ std::move(source_sheets) }
	};
}
SpriteSheetMerger::TargetSheet SpriteSheetMerger::MakePetsTargetSheet() {
	std::vector<std::pair<std::string_view, std::uint32_t>> name_to_idx{
		{ "monty", 0 },
		{ "percy", 1 },
		{ "poochi", 2 }
	};

	std::vector<SourceSheet> source_sheets;
	for (const auto& [pet_name, idx] : name_to_idx) {
		source_sheets.push_back(SourceSheet{
				.Path{ fmt::format("Data/Textures/Merged/{}_full.png", pet_name) },
				.Size{.Width{ 1536 }, .Height{ 672 } },
				.TileMap = std::vector<TileMapping>{
					TileMapping{
						.SourceTile{ 0, 0, 1536, 512 },
						.TargetTile{ 0, 512 * idx, 1536, 512 * idx + 512 },
					}
				}
			});
	}

	return TargetSheet{
		.Path{ "Data/Textures/monsters_pets.png" },
		.Size{ .Width{ 1536 }, .Height{ 1536 } },
		.SourceSheets{ std::move(source_sheets) }
	};
}
SpriteSheetMerger::TargetSheet SpriteSheetMerger::MakeCharacterTargetSheet(std::string_view color) {
	const bool is_npc = color == "hired" || color == "eggchild";
	const std::uint32_t image_height = is_npc ? 2080 : 2160;
	std::vector<SourceSheet> source_sheets{
		SourceSheet{
			.Path{ fmt::format("Data/Textures/Merged/char_{}_full.png", color) },
			.Size{.Width{ 2048 }, .Height{ image_height } },
			.TileMap = std::vector<TileMapping>{
				TileMapping{
					.SourceTile{ 0, 0, 2048, 1920 },
					.TargetTile{ 0, 0, 2048, 1920 },
				}
			}
		}
	};
	return TargetSheet{
		.Path{ fmt::format("Data/Textures/char_{}.png", color) },
		.Size{ .Width{ 2048 }, .Height{ 2048 } },
		.SourceSheets{ std::move(source_sheets) }
	};
}
