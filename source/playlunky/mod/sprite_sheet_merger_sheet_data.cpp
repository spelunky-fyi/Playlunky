#include "sprite_sheet_merger.h"

#include "generate_sticker_pixel_art.h"
#include "util/format.h"

// Note: All the `TileMap = std::vector<TileMapping>{ ... }` code is because of a bug in MSVC
void SpriteSheetMerger::MakeItemsSheet() {
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

	m_TargetSheets.push_back(TargetSheet{
		.Path{ "Data/Textures/items.png" },
		.Size{ .Width{ 2048 }, .Height{ 2048 } },
		.SourceSheets{ std::move(source_sheets) }
	});
}
void SpriteSheetMerger::MakeJournalItemsSheet() {
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

	m_TargetSheets.push_back(TargetSheet{
		.Path{ "Data/Textures/journal_entry_items.png" },
		.Size{ .Width{ 1600 }, .Height{ 1600 } },
		.SourceSheets{ std::move(source_sheets) }
	});
}
void SpriteSheetMerger::MakeJournalMonstersSheet() {
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

	m_TargetSheets.push_back(TargetSheet{
		.Path{ "Data/Textures/journal_entry_mons.png" },
		.Size{ .Width{ 1600 }, .Height{ 960 } },
		.SourceSheets{ std::move(source_sheets) }
	});
}
void SpriteSheetMerger::MakeJournalPeopleSheet() {
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

	m_TargetSheets.push_back(TargetSheet{
		.Path{ "Data/Textures/journal_entry_people.png" },
		.Size{ .Width{ 1600 }, .Height{ 800 } },
		.SourceSheets{ std::move(source_sheets) },
		.RandomSelect{ mRandomCharacterSelectEnabled }
	});
}
void SpriteSheetMerger::MakeJournalStickerSheet() {
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
								.SourceTile{ 256, 1152, 384, 1280 },
								.TargetTile{ char_x * 80, char_y * 80, char_x * 80 + 80, char_y * 80 + 80 },
							}
						},
						.Processing{ mGenerateStickerPixelArtEnabled ? GenerateStickerPixelArt : nullptr }
					});
			}
			char_x++;
			if (char_x >= 10) {
				char_x = 0;
				char_y++;
			}
		}
	}

	m_TargetSheets.push_back(TargetSheet{
		.Path{ "Data/Textures/journal_stickers.png" },
		.Size{ .Width{ 800 }, .Height{ 800 } },
		.SourceSheets{ std::move(source_sheets) },
		.RandomSelect{ mRandomCharacterSelectEnabled }
	});
}
void SpriteSheetMerger::MakeMountsTargetSheet() {
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

	m_TargetSheets.push_back(TargetSheet{
		.Path{ "Data/Textures/mounts.png" },
		.Size{ .Width{ 2048 }, .Height{ 2048 } },
		.SourceSheets{ std::move(source_sheets) }
	});
}
void SpriteSheetMerger::MakePetsTargetSheet() {
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

	m_TargetSheets.push_back(TargetSheet{
		.Path{ "Data/Textures/monsters_pets.png" },
		.Size{ .Width{ 1536 }, .Height{ 1536 } },
		.SourceSheets{ std::move(source_sheets) }
	});
}
void SpriteSheetMerger::MakeCharacterTargetSheet(std::string_view color) {
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
	m_TargetSheets.push_back(TargetSheet{
		.Path{ fmt::format("Data/Textures/char_{}.png", color) },
		.Size{ .Width{ 2048 }, .Height{ 2048 } },
		.SourceSheets{ std::move(source_sheets) },
		.RandomSelect{ mRandomCharacterSelectEnabled }
	});
}
void SpriteSheetMerger::MakeMenuLeaderTargetSheet() {
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

	m_TargetSheets.push_back(TargetSheet{
		.Path{ "Data/Textures/menu_leader.png" },
		.Size{ .Width{ 1280 }, .Height{ 1280 } },
		.SourceSheets{ std::move(source_sheets) },
		.RandomSelect{ mRandomCharacterSelectEnabled }
	});
}
