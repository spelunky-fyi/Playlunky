#include "sprite_sheet_merger.h"

#include "png_dds_conversion.h"
#include "virtual_filesystem.h"
#include "playlunky_settings.h"
#include "util/algorithms.h"
#include "util/image.h"
#include "util/format.h"

#include <cassert>

#pragma warning(push)
#pragma warning(disable : 5054)
#pragma warning(disable : 4996)
#pragma warning(disable : 4244)
#include <opencv2/imgproc.hpp>
#pragma warning(pop)

SpriteSheetMerger::SpriteSheetMerger(const PlaylunkySettings& settings)
	: mRandomCharacterSelectEnabled{ settings.GetBool("settings", "random_character_select", false) }
	, mGenerateCharacterJournalStickersEnabled{ settings.GetBool("sprite_settings", "generate_character_journal_stickers", false) }
	, mGenerateCharacterJournalEntriesEnabled{ settings.GetBool("sprite_settings", "generate_character_journal_entries", false) }
	, m_TargetSheets{
		MakeItemsSheet(),
		MakeJournalItemsSheet(),
		MakeJournalMonstersSheet(),
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
		MakeMenuLeaderTargetSheet()
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
							.x{ static_cast<std::uint32_t>(tile_mapping.SourceTile.Left * source_width_scaling) },
							.y{ static_cast<std::uint32_t>(tile_mapping.SourceTile.Top * source_height_scaling) },
							.width{ static_cast<std::uint32_t>((tile_mapping.SourceTile.Right - tile_mapping.SourceTile.Left) * source_width_scaling) },
							.height{ static_cast<std::uint32_t>((tile_mapping.SourceTile.Bottom - tile_mapping.SourceTile.Top) * source_height_scaling) },
						};
						const ImageSubRegion target_region = ImageSubRegion{
							.x{ static_cast<std::uint32_t>(tile_mapping.TargetTile.Left * target_height_scaling) },
							.y{ static_cast<std::uint32_t>(tile_mapping.TargetTile.Top * source_height_scaling) },
							.width{ static_cast<std::uint32_t>((tile_mapping.TargetTile.Right - tile_mapping.TargetTile.Left) * target_height_scaling) },
							.height{ static_cast<std::uint32_t>((tile_mapping.TargetTile.Bottom - tile_mapping.TargetTile.Top) * source_height_scaling) },
						};

						Image source_tile = source_image.GetSubImage(source_region);
						if (source_tile.GetWidth() != target_region.width || source_tile.GetHeight() != target_region.height) {
							source_tile.Resize(::ImageSize{ .x = target_region.width, .y = target_region.height });
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

// Note: All the `TileMap = std::vector<TileMapping>{ ... }` code is because of a bug in MSVC
SpriteSheetMerger::TargetSheet SpriteSheetMerger::MakeItemsSheet() {
	std::vector<SourceSheet> source_sheets;

	source_sheets.push_back(SourceSheet{
			.Path{ "Data/Textures/Entities/turkey_full.png" },
			.Size{ .Width{ 2048 }, .Height{ 960 } },
			.TileMap = std::vector<TileMapping>{
				TileMapping{
					.SourceTile{ 0, 832, 128, 960 },
					.TargetTile{ 1920, 1792, 2048, 1920 },
				}
			}
		});

	return TargetSheet{
		.Path{ "Data/Textures/items.png" },
		.Size{ .Width{ 2048 }, .Height{ 2048 } },
		.SourceSheets{ std::move(source_sheets) }
	};
}
SpriteSheetMerger::TargetSheet SpriteSheetMerger::MakeJournalItemsSheet() {
	std::vector<SourceSheet> source_sheets;

	source_sheets.push_back(SourceSheet{
			.Path{ "Data/Textures/Entities/turkey_full.png" },
			.Size{ .Width{ 2048 }, .Height{ 960 } },
			.TileMap = std::vector<TileMapping>{
				TileMapping{
					.SourceTile{ 0, 672, 160, 832 },
					.TargetTile{ 800, 480, 960, 640 },
				}
			}
		});

	return TargetSheet{
		.Path{ "Data/Textures/journal_entry_items.png" },
		.Size{ .Width{ 1600 }, .Height{ 1600 } },
		.SourceSheets{ std::move(source_sheets) }
	};
}
SpriteSheetMerger::TargetSheet SpriteSheetMerger::MakeJournalMonstersSheet() {
	std::vector<SourceSheet> source_sheets;

	source_sheets.push_back(SourceSheet{
			.Path{ "Data/Textures/Entities/monty_full.png" },
			.Size{ .Width{ 1536 }, .Height{ 672 } },
			.TileMap = std::vector<TileMapping>{
				TileMapping{
					.SourceTile{ 0, 512, 160, 672 },
					.TargetTile{ 800, 480, 960, 640 },
				}
			}
		});
	source_sheets.push_back(SourceSheet{
			.Path{ "Data/Textures/Entities/percy_full.png" },
			.Size{ .Width{ 1536 }, .Height{ 672 } },
			.TileMap = std::vector<TileMapping>{
				TileMapping{
					.SourceTile{ 0, 512, 160, 672 },
					.TargetTile{ 1440, 480, 1600, 640 },
				}
			}
		});
	source_sheets.push_back(SourceSheet{
			.Path{ "Data/Textures/Entities/poochi_full.png" },
			.Size{ .Width{ 1536 }, .Height{ 672 } },
			.TileMap = std::vector<TileMapping>{
				TileMapping{
					.SourceTile{ 0, 512, 160, 672 },
					.TargetTile{ 640, 480, 800, 640 },
				}
			}
		});
	source_sheets.push_back(SourceSheet{
			.Path{ "Data/Textures/Entities/turkey_full.png" },
			.Size{ .Width{ 2048 }, .Height{ 960 } },
			.TileMap = std::vector<TileMapping>{
				TileMapping{
					.SourceTile{ 0, 512, 160, 672 },
					.TargetTile{ 1280, 640, 1440, 800},
				}
			}
		});
	source_sheets.push_back(SourceSheet{
			.Path{ "Data/Textures/Entities/rockdog_full.png" },
			.Size{ .Width{ 1920 }, .Height{ 672 } },
			.TileMap = std::vector<TileMapping>{
				TileMapping{
					.SourceTile{ 0, 512, 160, 672 },
					.TargetTile{ 1440, 640, 1600, 800},
				}
			}
		});
	source_sheets.push_back(SourceSheet{
			.Path{ "Data/Textures/Entities/axolotl_full.png" },
			.Size{ .Width{ 1920 }, .Height{ 672 } },
			.TileMap = std::vector<TileMapping>{
				TileMapping{
					.SourceTile{ 0, 512, 160, 672 },
					.TargetTile{ 0, 800, 160, 960 },
				}
			}
		});
	source_sheets.push_back(SourceSheet{
			.Path{ "Data/Textures/Entities/qilin_full.png" },
			.Size{ .Width{ 1920 }, .Height{ 672 } },
			.TileMap = std::vector<TileMapping>{
				TileMapping{
					.SourceTile{ 0, 512, 160, 672 },
					.TargetTile{ 160, 800, 320, 960 },
				}
			}
		});

	return TargetSheet{
		.Path{ "Data/Textures/journal_entry_mons.png" },
		.Size{ .Width{ 1600 }, .Height{ 960 } },
		.SourceSheets{ std::move(source_sheets) }
	};
}
SpriteSheetMerger::TargetSheet SpriteSheetMerger::MakeJournalPeopleSheet() {
	std::vector<SourceSheet> source_sheets;

	{
		std::uint32_t char_x{ 0 };
		std::uint32_t char_y{ 0 };
		for (std::string_view color : { "yellow", "magenta", "cyan", "black", "cinnabar", "green", "olive", "white", "cerulean", "blue",
			"lime", "lemon", "iris", "gold", "red", "pink", "violet", "gray", "khaki", "orange" }) {
			source_sheets.push_back(SourceSheet{
					.Path{ fmt::format("Data/Textures/Entities/char_{}_full.png", color) },
					.Size{ .Width{ 2048 }, .Height{ 2160 } },
					.TileMap = std::vector<TileMapping>{
						TileMapping{
							.SourceTile{ 0, 1920, 160, 2080 },
							.TargetTile{ char_x * 160, char_y * 160, char_x * 160 + 160, char_y * 160 + 160 },
						}
					}
				});
			source_sheets.push_back(source_sheets.back());
			source_sheets.back().Size.Height = 2224;
			if (mGenerateCharacterJournalEntriesEnabled) {
				source_sheets.push_back(SourceSheet{
						.Path{ fmt::format("Data/Textures/char_{}.png", color) },
						.Size{ .Width{ 2048 }, .Height{ 2048 } },
						.TileMap = std::vector<TileMapping>{
							TileMapping{
								.SourceTile{ 0, 0, 128, 128 },
								.TargetTile{ char_x * 160, char_y * 160, char_x * 160 + 160, char_y * 160 + 160 },
							}
						}
					});
			}
			char_x++;
			if (char_x >= 10) {
				char_x = 0;
				char_y++;
			}
		}
	}
		
	source_sheets.push_back(SourceSheet{
			.Path{ "Data/Textures/Entities/char_eggchild_full.png" },
			.Size{ .Width{ 2048 }, .Height{ 2080 } },
			.TileMap = std::vector<TileMapping>{
				TileMapping{
					.SourceTile{ 0, 1920, 160, 2080 },
					.TargetTile{ 160, 320, 320, 480 },
				}
			}
		});
	if (mGenerateCharacterJournalEntriesEnabled) {
		source_sheets.push_back(SourceSheet{
				.Path{ "Data/Textures/char_eggchild.png" },
				.Size{ .Width{ 2048 }, .Height{ 2048 } },
				.TileMap = std::vector<TileMapping>{
					TileMapping{
						.SourceTile{ 0, 0, 128, 128 },
						.TargetTile{ 160, 320, 320, 480 },
					}
				}
			});
	}
	source_sheets.push_back(SourceSheet{
			.Path{ "Data/Textures/Entities/char_hired_full.png" },
			.Size{ .Width{ 2048 }, .Height{ 2080 } },
			.TileMap = std::vector<TileMapping>{
				TileMapping{
					.SourceTile{ 0, 1920, 160, 2080 },
					.TargetTile{ 0, 320, 160, 480 },
				}
			}
		});
	if (mGenerateCharacterJournalEntriesEnabled) {
		source_sheets.push_back(SourceSheet{
				.Path{ "Data/Textures/Entities/char_hired.png" },
				.Size{ .Width{ 2048 }, .Height{ 2048 } },
				.TileMap = std::vector<TileMapping>{
					TileMapping{
						.SourceTile{ 0, 0, 128, 128 },
						.TargetTile{ 0, 320, 160, 480 },
					}
				}
			});
	}

	return TargetSheet{
		.Path{ "Data/Textures/journal_entry_people.png" },
		.Size{ .Width{ 1600 }, .Height{ 800 } },
		.SourceSheets{ std::move(source_sheets) },
		.RandomSelect{ mRandomCharacterSelectEnabled }
	};
}
SpriteSheetMerger::TargetSheet SpriteSheetMerger::MakeJournalStickerSheet() {
	std::vector<SourceSheet> source_sheets;

	{
		std::uint32_t char_x{ 0 };
		std::uint32_t char_y{ 0 };
		for (std::string_view color : { "yellow", "magenta", "cyan", "black", "cinnabar", "green", "olive", "white", "cerulean", "blue",
			"lime", "lemon", "iris", "gold", "red", "pink", "violet", "gray", "khaki", "orange" }) {
			source_sheets.push_back(SourceSheet{
					.Path{ fmt::format("Data/Textures/Entities/char_{}_full.png", color) },
					.Size{ .Width{ 2048 }, .Height{ 2160 } },
					.TileMap = std::vector<TileMapping>{
						TileMapping{
							.SourceTile{ 0, 2080, 80, 2160 },
							.TargetTile{ char_x * 80, char_y * 80, char_x * 80 + 80, char_y * 80 + 80 },
						}
					}
				});
			source_sheets.push_back(source_sheets.back());
			source_sheets.back().Size.Height = 2224;
			if (mGenerateCharacterJournalStickersEnabled) {
				source_sheets.push_back(SourceSheet{
						.Path{ fmt::format("Data/Textures/char_{}.png", color) },
						.Size{ .Width{ 2048 }, .Height{ 2048 } },
						.TileMap = std::vector<TileMapping>{
							TileMapping{
								.SourceTile{ 0, 0, 128, 128 },
								.TargetTile{ char_x * 80, char_y * 80, char_x * 80 + 80, char_y * 80 + 80 },
							}
						}
					});
			}
			char_x++;
			if (char_x >= 10) {
				char_x = 0;
				char_y++;
			}
		}
	}

	return TargetSheet{
		.Path{ "Data/Textures/journal_stickers.png" },
		.Size{ .Width{ 800 }, .Height{ 800 } },
		.SourceSheets{ std::move(source_sheets) },
		.RandomSelect{ mRandomCharacterSelectEnabled }
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
		const std::uint32_t image_width = pet_name == "turkey" ? 2048 : 1920;
		const std::uint32_t image_height = pet_name == "turkey" ? 960 : 672;
		source_sheets.push_back(SourceSheet{
				.Path{ fmt::format("Data/Textures/Entities/{}_full.png", pet_name) },
				.Size{ .Width{ image_width }, .Height{ image_height } },
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
		.Size{ .Width{ 2048 }, .Height{ 2048 } },
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
				.Path{ fmt::format("Data/Textures/Entities/{}_full.png", pet_name) },
				.Size{ .Width{ 1536 }, .Height{ 672 } },
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
	const std::uint32_t old_image_height = is_npc ? 2080 : 2160;
	const std::uint32_t image_height = is_npc ? 2080 : 2224;
	std::vector<SourceSheet> source_sheets{
		SourceSheet{
			.Path{ fmt::format("Data/Textures/Entities/char_{}_full.png", color) },
			.Size{ .Width{ 2048 }, .Height{ old_image_height } },
			.TileMap = std::vector<TileMapping>{
				TileMapping{
					.SourceTile{ 0, 0, 2048, 1920 },
					.TargetTile{ 0, 0, 2048, 1920 },
				}
			}
		},
		SourceSheet{
			.Path{ fmt::format("Data/Textures/char_{}.png", color) },
			.Size{ .Width{ 2048 }, .Height{ 2048 } },
			.TileMap = std::vector<TileMapping>{
				TileMapping{
					.SourceTile{ 0, 0, 2048, 1920 },
					.TargetTile{ 0, 0, 2048, 1920 },
				}
			}
		}
	};
	source_sheets.push_back(source_sheets.front());
	source_sheets.back().Size.Height = 2224;
	return TargetSheet{
		.Path{ fmt::format("Data/Textures/char_{}.png", color) },
		.Size{ .Width{ 2048 }, .Height{ 2048 } },
		.SourceSheets{ std::move(source_sheets) },
		.RandomSelect{ mRandomCharacterSelectEnabled }
	};
}
SpriteSheetMerger::TargetSheet SpriteSheetMerger::MakeMenuLeaderTargetSheet() {
	std::vector<SourceSheet> source_sheets;

	{
		std::uint32_t char_x{ 0 };
		std::uint32_t char_y{ 0 };
		for (std::string_view color : { "yellow", "magenta", "cyan", "black", "cinnabar", "green", "olive", "white", "cerulean", "blue",
			"lime", "lemon", "iris", "gold", "red", "pink", "violet", "gray", "khaki", "orange" }) {
			source_sheets.push_back(SourceSheet{
					.Path{ fmt::format("Data/Textures/Entities/char_{}_full.png", color) },
					.Size{ .Width{ 2048 }, .Height{ 2224 } },
					.TileMap = std::vector<TileMapping>{
						TileMapping{
							.SourceTile{ 0, 2160, 128, 2224 },
							.TargetTile{ char_x * 128, 448 + char_y * 128, char_x * 128 + 128, 448 + char_y * 128 + 64 },
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
		.Path{ "Data/Textures/menu_leader.png" },
		.Size{ .Width{ 1280 }, .Height{ 1280 } },
		.SourceSheets{ std::move(source_sheets) },
		.RandomSelect{ mRandomCharacterSelectEnabled }
	};
}
