#include "sprite_sheet_merger.h"

#include "entity_data_extraction.h"
#include "generate_sticker_pixel_art.h"
#include "util/algorithms.h"
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
					.TargetTile{ 1280, 640, 1440, 800 },
				}
			}
		});
	source_sheets.push_back(SourceSheet{
			.Path{ "Data/Textures/Entities/rockdog_full.png" },
			.Size{ .Width{ 1920 }, .Height{ 672 } },
			.TileMap = std::vector<TileMapping>{
				TileMapping{
					.SourceTile{ 0, 512, 160, 672 },
					.TargetTile{ 1440, 640, 1600, 800 },
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

	{
		struct MonsterJournalEntry {
			std::string_view SubPath;
			Tile TargetTile;
		};
		std::vector<MonsterJournalEntry> entries{
			{ "Pets/monty.png", { 800, 480, 960, 640 } },
			{ "Pets/percy.png", { 1440, 480, 1600, 640 } },
			{ "Pets/poochi.png", { 640, 480, 800, 640 } },

			{ "Mounts/turkey.png", { 1280, 640, 1440, 800 } },
			{ "Mounts/rockdog.png", { 1440, 640, 1600, 800 } },
			{ "Mounts/axolotl.png", { 0, 800, 160, 960 } },
			{ "Mounts/qilin.png", { 160, 800, 320, 960 } },

			{ "Monsters/snake.png", { 0, 0, 160, 160 } },
			{ "Monsters/spider.png", { 160, 0, 320, 160 } },
			{ "Monsters/hang_spider.png", { 320, 0, 480, 160 } },
			{ "Monsters/bat.png", { 480, 0, 640, 160 } },
			{ "Monsters/cave_man.png", { 640, 0, 800, 160 } },
			{ "Monsters/skeleton.png", { 800, 0, 960, 160 } },
			{ "Monsters/scorpion.png", { 960, 0, 1120, 160 } },
			{ "Monsters/horned_lizard.png", { 1120, 0, 1280, 160 } },
			{ "Monsters/man_trap.png", { 1280, 0, 1440, 160 } },
			{ "Monsters/tiki_man.png", { 1440, 0, 1600, 160 } },
			{ "Monsters/witch_doctor.png", { 0, 160, 160, 320 } },
			{ "Monsters/mosquito.png", { 160, 160, 320, 320 } },
			{ "Monsters/monkey.png", { 320, 160, 480, 320 } },
			{ "Monsters/magmar.png", { 480, 160, 640, 320 } },
			{ "Monsters/robot.png", { 640, 160, 800, 320 } },
			{ "Monsters/fire_bug.png", { 800, 160, 960, 320 } },
			{ "Monsters/imp.png", { 960, 160, 1120, 320 } },
			{ "Monsters/vampire.png", { 1120, 160, 1280, 320 } },
			{ "Monsters/vlad.png", { 1280, 160, 1440, 320 } },
			{ "Monsters/croc_man.png", { 1440, 160, 1600, 320 } },
			{ "Monsters/cobra.png", { 0, 320, 160, 480 } },
			{ "Monsters/sorceress.png", { 160, 320, 320, 480 } },
			{ "Monsters/cat_mummy.png", { 320, 320, 480, 480 } },
			{ "Monsters/jiangshi.png", { 480, 320, 640, 480 } },
			{ "Monsters/flying_fish.png", { 640, 320, 800, 480 } },
			{ "Monsters/octopus.png", { 800, 320, 960, 480 } },
			{ "Monsters/hermit_crab.png", { 960, 320, 1120, 480 } },
			{ "Monsters/ufo.png", { 1120, 320, 1280, 480 } },
			{ "Monsters/alien.png", { 1280, 320, 1440, 480 } },
			{ "Monsters/yeti.png", { 1440, 320, 1600, 480 } },
			{ "Monsters/olmite_naked.png", { 0, 480, 160, 640 } },
			{ "Monsters/necromancer.png", { 160, 480, 320, 640 } },
			{ "Monsters/bee.png", { 320, 480, 480, 640 } },
			{ "Monsters/golden_monkey.png", { 960, 480, 1120, 640 } },
			{ "Monsters/female_jiangshi.png", { 1120, 480, 1280, 640 } },
			{ "Monsters/mole.png", { 1280, 480, 1440, 640 } },
			{ "Monsters/proto_shopkeeper.png", { 0, 640, 160, 800 } },
			{ "Monsters/grub.png", { 160, 640, 320, 800 } },
			{ "Monsters/frog.png", { 320, 640, 480, 800 } },
			{ "Monsters/fire_frog.png", { 480, 640, 640, 800 } },
			{ "Monsters/leprechaun.png", { 640, 640, 800, 800 } },
			{ "Monsters/jumpdog.png", { 800, 640, 960, 800 } },
			{ "Monsters/tadpole.png", { 1120, 640, 1280, 800 } },

			{ "Ghost/ghist.png", { 960, 640, 1120, 800 } },
		};
		for (const auto& entry : entries) {
			if (auto mapping = m_EntityDataExtractor->GetAdditionalMapping(fmt::format("Data/Textures/Entities/{}", entry.SubPath),
				Tile{ 0, 0, 160, 160 },
				entry.TargetTile)) {
				source_sheets.push_back(std::move(mapping).value());
			}
		}
	}

	m_TargetSheets.push_back(TargetSheet{
		.Path{ "Data/Textures/journal_entry_mons.png" },
		.Size{ .Width{ 1600 }, .Height{ 960 } },
		.SourceSheets{ std::move(source_sheets) }
	});
}
void SpriteSheetMerger::MakeJournalMonstersBigSheet() {
	std::vector<SourceSheet> source_sheets;

	{
		struct BigMonsterJournalEntry {
			std::string_view SubPath;
			Tile TargetTile;
		};
		std::vector<BigMonsterJournalEntry> entries{
			{ "Ghost/ghost.png", { 320, 320, 640, 640 } },

			{ "BigMonsters/quill_back.png", { 0, 0, 320, 320 } },
			{ "BigMonsters/mummy.png", { 640, 0, 960, 320 } },
			{ "BigMonsters/lamassu.png", { 1280, 0, 1600, 320 } },
			{ "BigMonsters/queen_bee.png", { 0, 320, 320, 640 } },
			{ "BigMonsters/yeti_king.png", { 960, 320, 1280, 640 } },
			{ "BigMonsters/yeti_queen.png", { 1280, 320, 1600, 640 } },
			{ "BigMonsters/giant_frog.png", { 0, 640, 320, 960 } },
			{ "BigMonsters/giant_spider.png", { 320, 640, 640, 960 } },
			{ "BigMonsters/lavamander.png", { 640, 640, 960, 960 } },
			{ "BigMonsters/crab_man.png", { 960, 640, 1280, 960 } },
			{ "BigMonsters/eggplant_minister.png", { 0, 960, 320, 1280 } },
			{ "BigMonsters/giant_fish.png", { 640, 960, 960, 1280 } },
			{ "BigMonsters/alien_queen.png", { 960, 960, 1280, 1280 } },
			{ "BigMonsters/ammit.png", { 1280, 960, 1600, 1280 } },
			{ "BigMonsters/osiris.png", { 960, 1280, 1280, 1600 } },
			{ "BigMonsters/giant_fly.png", { 0, 1600, 320, 1920 } },
			//{ "BigMonsters/olmec.png", { 320, 0, 640, 320 } },
			//{ "BigMonsters/celestial_jellyfish.png", { 1280, 640, 1600, 960 } },
			//{ "Mounts/mech.png", { 320, 960, 640, 1280 } },
			//{ "BigMonsters/anubis.png", { 960, 0, 1280, 320 } },
			//{ "BigMonsters/anubis_2.png", { 0, 1280, 320, 1600 } },
			//{ "BigMonsters/hundun.png", { 320, 1280, 640, 1600 } },
			//{ "BigMonsters/kingu.png", { 640, 1280, 960, 1600 } },
			//{ "BigMonsters/tiamat.png", { 1280, 1280, 1600, 1600 } },
		};
		for (const auto& entry : entries) {
			if (auto mapping = m_EntityDataExtractor->GetAdditionalMapping(fmt::format("Data/Textures/Entities/{}", entry.SubPath),
				Tile{ 0, 0, 320, 320 },
				entry.TargetTile)) {
				source_sheets.push_back(std::move(mapping).value());
			}
		}
	}

	m_TargetSheets.push_back(TargetSheet{
		.Path{ "Data/Textures/journal_entry_mons_big.png" },
		.Size{ .Width{ 1600 }, .Height{ 1920 } },
		.SourceSheets{ std::move(source_sheets) },
		.RandomSelect{ false }
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

	{
		struct PeopleJournalEntry {
			std::string_view SubPath;
			Tile TargetTile;
		};
		std::vector<PeopleJournalEntry> entries{
			{ "Ghost/ghist.png", { 480, 480, 640, 640 } },

			{ "Monsters/cave_man.png", { 320, 480, 480, 640 } },

			{ "People/shopkeeper.png",{ 320, 320, 480, 480 } },
			{ "People/merchant.png", { 480, 320, 640, 480 } },
			{ "People/yang.png", { 640, 320, 800, 480 } },
			{ "People/old_hunter.png", { 800, 320, 960, 480 } },
			{ "People/thief.png", { 960, 320, 1120, 480 } },
			{ "People/parsley.png", { 1120, 320, 1280, 480 } },
			{ "People/parsnip.png", { 1280, 320, 1440, 480 } },
			{ "People/parmesan.png", { 1440, 320, 1600, 480 } },
			{ "People/hunduns_servant.png", { 0, 480, 160, 640 } },
			{ "People/bodyguard.png", { 160, 480, 320, 640 } },
			//{ "People/mama_tunnel.png", { 0, 640, 160, 800 } },
		};
		for (const auto& entry : entries) {
			if (auto mapping = m_EntityDataExtractor->GetAdditionalMapping(fmt::format("Data/Textures/Entities/{}", entry.SubPath),
				Tile{ 0, 0, 160, 160 },
				entry.TargetTile)) {
				source_sheets.push_back(std::move(mapping).value());
			}
		}
	}

	{
		struct BigPeopleJournalEntry {
			std::string_view SubPath;
			Tile TargetTile;
		};
		std::vector<BigPeopleJournalEntry> entries{
			//{ "BigMonsters/yama.png", { 640, 480, 960, 800 } },
			{ "BigMonsters/madame_tusk.png", { 960, 480, 1280, 800 } },
			{ "BigMonsters/waddler.png", { 1280, 480, 1600, 800 } },
		};
		for (const auto& entry : entries) {
			if (auto mapping = m_EntityDataExtractor->GetAdditionalMapping(fmt::format("Data/Textures/Entities/{}", entry.SubPath),
				Tile{ 0, 0, 320, 320 },
				entry.TargetTile)) {
				source_sheets.push_back(std::move(mapping).value());
			}
		}
	}

	m_TargetSheets.push_back(TargetSheet{
		.Path{ "Data/Textures/journal_entry_people.png" },
		.Size{.Width{ 1600 }, .Height{ 800 } },
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
					.Size{.Width{ 2048 }, .Height{ 2160 } },
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
						.Size{.Width{ 2048 }, .Height{ 2048 } },
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

	{
		struct StickerEntry {
			std::string_view SubPath;
			Tile TargetTile;
		};
		std::vector<StickerEntry> entries{
			{ "Monsters/cave_man.png", { 160, 400, 240, 480 } },
			{ "Monsters/vlad.png", { 720, 400, 800, 480 } },

			{ "People/shopkeeper.png", { 0, 400, 80, 480 } },
			{ "People/merchant.png", { 80, 400, 160, 480 } },
			{ "People/yang.png", { 240, 400, 320, 480 } },
			{ "People/parsley.png", { 320, 400, 400, 480 } },
			{ "People/parsnip.png", { 400, 400, 480, 480 } },
			{ "People/parmesan.png", { 480, 400, 560, 480 } },
			{ "People/old_hunter.png", { 560, 400, 640, 480 } },
			{ "People/thief.png", { 640, 400, 720, 480 } },
		};
		for (const auto& entry : entries) {
			if (auto mapping = m_EntityDataExtractor->GetAdditionalMapping(fmt::format("Data/Textures/Entities/{}", entry.SubPath),
				Tile{ 0, 160, 80, 240 },
				entry.TargetTile)) {
				source_sheets.push_back(std::move(mapping).value());
			}
		}
	}

	{
		struct BigStickerEntry {
			std::string_view SubPath;
			Tile TargetTile;
		};
		std::vector<BigStickerEntry> entries{
			{ "BigMonsters/quill_back.png", { 0, 480, 160, 640 } },
			//{ "BigMonsters/kingu.png", { 160, 480, 320, 640 } },
			//{ "BigMonsters/anubis.png", { 320, 480, 480, 640 } },
			{ "BigMonsters/osiris.png", { 480, 480, 640, 640 } },
			{ "BigMonsters/alien_queen.png", { 640, 480, 800, 640 } },
			//{ "BigMonsters/olmec.png", { 0, 640, 160, 800 } },
			//{ "BigMonsters/tiamat.png", { 160, 640, 320, 800 } },
			//{ "BigMonsters/yama.png", { 320, 640, 480, 800 } },
			//{ "BigMonsters/hundun.png", { 480, 640, 640, 800 } },
		};
		for (const auto& entry : entries) {
			if (auto mapping = m_EntityDataExtractor->GetAdditionalMapping(fmt::format("Data/Textures/Entities/{}", entry.SubPath),
				Tile{ 0, 320, 160, 480 },
				entry.TargetTile)) {
				source_sheets.push_back(std::move(mapping).value());
			}
		}
	}

	m_TargetSheets.push_back(TargetSheet{
		.Path{ "Data/Textures/journal_stickers.png" },
		.Size{.Width{ 800 }, .Height{ 800 } },
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
	for (const auto& [mount_name, idx] : name_to_idx) {
		const std::uint32_t image_width = mount_name == "turkey" ? 2048 : 1920;
		const std::uint32_t image_height = mount_name == "turkey" ? 960 : 672;
		source_sheets.push_back(SourceSheet{
				.Path{ fmt::format("Data/Textures/Entities/{}_full.png", mount_name) },
				.Size{.Width{ image_width }, .Height{ image_height } },
				.TileMap = std::vector<TileMapping>{
					TileMapping{
						.SourceTile{ 0, 0, 1536, 512 },
						.TargetTile{ 0, 512 * idx, 1536, 512 * idx + 512 },
					}
				}
			});
		if (auto sheet = m_EntityDataExtractor->GetEntitySourceSheet(fmt::format("Data/Textures/Entities/Mounts/{}.png", mount_name))) {
			if (mount_name == "turkey") {
				sheet->TileMap.push_back(TileMapping{ .SourceTile{ 0, 0, 512, 128 }, .TargetTile{ 1536, 128, 2048, 256 } });
			}
			source_sheets.push_back(std::move(sheet).value());
		}
	}

	m_TargetSheets.push_back(TargetSheet{
		.Path{ "Data/Textures/mounts.png" },
		.Size{.Width{ 2048 }, .Height{ 2048 } },
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
				.Size{.Width{ 1536 }, .Height{ 672 } },
				.TileMap = std::vector<TileMapping>{
					TileMapping{
						.SourceTile{ 0, 0, 1536, 512 },
						.TargetTile{ 0, 512 * idx, 1536, 512 * idx + 512 },
					}
				}
			});
		if (auto sheet = m_EntityDataExtractor->GetEntitySourceSheet(fmt::format("Data/Textures/Entities/Pets/{}.png", pet_name))) {
			source_sheets.push_back(std::move(sheet).value());
		}
	}

	m_TargetSheets.push_back(TargetSheet{
		.Path{ "Data/Textures/monsters_pets.png" },
		.Size{.Width{ 1536 }, .Height{ 1536 } },
		.SourceSheets{ std::move(source_sheets) }
		});
}
void SpriteSheetMerger::MakeMonstersTargetSheet() {
	struct AdditionalMapping {
		std::string_view SourceFile;
		std::vector<TileMapping> TileMap;
	};
	struct MonsterSheet {
		std::string_view TargetFile;
		std::vector<std::string_view> SourceFiles;
		std::vector<AdditionalMapping> AdditionalTileMaps;
		std::vector<SourceSheet> AdditionalSources;
		SheetSize TargetSize{ .Width{ 2048 }, .Height{ 2048 } };
	};
	std::vector<MonsterSheet> sheets{
		{ "Data/Textures/monstersbasic01.png", {
				"Monsters/snake.png",
				"Monsters/bat.png",
				"Monsters/fly.png",
				"Monsters/skeleton.png",
				"Monsters/spider.png",
				"Monsters/ufo.png",
				"Monsters/alien.png",
				"Monsters/cobra.png",
				"Monsters/scorpion.png",
				"Monsters/golden_monkey.png",
				"Monsters/bee.png",
				"Monsters/magmar.png",

				"People/shopkeeper.png",
			}, {
				{ "Monsters/golden_monkey.png", {
						TileMapping{
							.SourceTile{ 0, 0, 128, 128 },
							.TargetTile{ 1408, 1280, 1536, 1408 }
						},
						TileMapping{
							.SourceTile{ 0, 0, 128, 128 },
							.TargetTile{ 1920, 1280, 2048, 1408 }
						}
					}
				}
			}
		},
		{ "Data/Textures/monstersbasic02.png", {
				"Monsters/vampire.png",
				"Monsters/vlad.png",
				"Monsters/cave_man.png",
				"Monsters/leprechaun.png",

				"People/bodyguard.png",
				"People/old_hunter.png",
				"People/merchant.png",
			}, {
				{ "Monsters/cave_man.png", {
						TileMapping{
							.SourceTile{ 0, 0, 1024, 128 },
							.TargetTile{ 0, 896, 1024, 1024 }
						},
						TileMapping{
							.SourceTile{ 0, 128, 256, 256 },
							.TargetTile{ 1024, 896, 1280, 1024 }
						}
					}
				}
			}
		},
		{ "Data/Textures/monstersbasic03.png", {
				"Critters/birdies.png",

				"People/hunduns_servant.png",
				"People/thief.png",
				"People/parmesan.png",
				"People/parsley.png",
				"People/parsnip.png",
				"People/yang.png",
			}
		},
		{ "Data/Textures/monsters01.png", {
				"Critters/snail.png",
				"Critters/dung_beetle.png",
				"Critters/butterfly.png",

				"Monsters/robot.png",
				"Monsters/imp.png",
				"Monsters/tiki_man.png",
				"Monsters/man_trap.png",
				"Monsters/fire_bug.png",
				"Monsters/mole.png",
				"Monsters/witch_doctor.png",
				"Monsters/horned_lizard.png",
				"Monsters/witch_doctor_skull.png",
				"Monsters/monkey.png",
				"Monsters/hang_spider.png",
				"Monsters/mosquito.png",
			}, {
				{ "Monsters/witch_doctor.png", {
						TileMapping{
							.SourceTile{ 0, 0, 256, 128 },
							.TargetTile{ 1536, 1280, 1792, 1408 }
						}
					}
				}
			}
		},
		{ "Data/Textures/monsters02.png", {
				"Critters/crab.png",
				"Critters/fish.png",
				"Critters/anchovy.png",
				"Critters/locust.png",

				"Monsters/jiangshi.png",
				"Monsters/hermit_crab.png",
				"Monsters/flying_fish.png",
				"Monsters/octopus.png",
				"Monsters/female_jiangshi.png",
				"Monsters/croc_man.png",
				"Monsters/sorceress.png",
				"Monsters/cat_mummy.png",
				"Monsters/necromancer.png",
			}, {
				{ "Monsters/jiangshi.png", {
						TileMapping{
							.SourceTile{ 0, 0, 128, 128 },
							.TargetTile{ 1152, 0, 1280, 128 }
						}
					}
				},
				{ "Monsters/female_jiangshi.png", {
						TileMapping{
							.SourceTile{ 0, 0, 128, 128 },
							.TargetTile{ 1152, 768, 1280, 896 }
						}
					}
				},
				{ "Monsters/hermit_crab.png", {
						TileMapping{
							.SourceTile{ 0, 0, 768, 128 },
							.TargetTile{ 1280, 512, 2048, 640 }
						},
						TileMapping{
							.SourceTile{ 768, 0, 896, 128 },
							.TargetTile{ 1408, 640, 1536, 768 }
						},
						TileMapping{
							.SourceTile{ 0, 128, 512, 256 },
							.TargetTile{ 1536, 640, 2048, 768 }
						}
					}
				}
			}, std::vector<SourceSheet>{
				SourceSheet{
					.Path{ "Data/Textures/Entities/Critters/blue_crab.png" },
					.Size{ 384, 256 },
					.TileMap = std::vector<TileMapping>{
						{ { 0, 0, 384, 128 }, { 768, 1664, 1152, 1792 } },
						{ { 0, 128, 384, 256 }, { 1152, 1664, 1526, 1792 } }
					}
				}
			}
		},
		{ "Data/Textures/monsters03.png", {
				"Critters/firefly.png",
				"Critters/penguin.png",
				"Critters/drone.png",
				"Critters/slime.png",

				"Monsters/yeti.png",
				"Monsters/proto_shopkeeper.png",
				"Monsters/jumpdog.png",
				"Monsters/tadpole.png",
				"Monsters/olmite_naked.png",
				"Monsters/grub.png",
				"Monsters/frog.png",
				"Monsters/fire_frog.png",
			}, {
				{ "Monsters/fire_frog.png", {
						TileMapping{
							.SourceTile{ 0, 0, 256, 128 },
							.TargetTile{ 640, 1538, 896, 1664 }
						}
					}
				}
			}, {
				SourceSheet{
					.Path{ "Data/Textures/Entities/Monsters/olmite_armored.png" },
					.Size{ 512, 384 },
					.TileMap = std::vector<TileMapping>{
						{ { 0, 0, 512, 128 }, { 0, 1024, 512, 1152 } },
						{ { 0, 128, 512, 256 }, { 512, 1024, 1024, 1152 } },
						{ { 0, 256, 256, 384 }, { 1024, 1024, 1280, 1152 } },
					}
				},
				SourceSheet{
					.Path{ "Data/Textures/Entities/Monsters/olmite_helmet.png" },
					.Size{ 512, 640 },
					.TileMap = std::vector<TileMapping>{
						{ { 0, 0, 512, 128 }, { 0, 1152, 512, 1280 } },
						{ { 0, 128, 512, 256 }, { 512, 1152, 1024, 1280 } },
						{ { 0, 256, 256, 384 }, { 1024, 1152, 1280, 1280 } },
						{ { 256, 256, 512, 384 }, { 0, 1280, 256, 1408 } },
						{ { 0, 384, 512, 512 }, { 256, 1280, 768, 1408 } },
						{ { 0, 512, 128, 640 }, { 768, 1280, 896, 1408 } }
					}
				},
			}
		},
	};

	for (const auto& sheet : sheets) {
		std::vector<SourceSheet> source_sheets = sheet.AdditionalSources;
		for (std::string_view source_file : sheet.SourceFiles) {
			if (auto mapping = m_EntityDataExtractor->GetEntitySourceSheet(fmt::format("Data/Textures/Entities/{}", source_file))) {
				if (auto additional_mapping = algo::find(sheet.AdditionalTileMaps, &AdditionalMapping::SourceFile, source_file)) {
					mapping->TileMap.insert(mapping->TileMap.end(), additional_mapping->TileMap.begin(), additional_mapping->TileMap.end());
				}
				source_sheets.push_back(std::move(mapping).value());
			}
		}
		m_TargetSheets.push_back(TargetSheet{
			.Path{ sheet.TargetFile },
			.Size{ sheet.TargetSize },
			.SourceSheets{ std::move(source_sheets) },
			.RandomSelect{ false }
			});
	}
}
void SpriteSheetMerger::MakeBigMonstersTargetSheet() {
	struct AdditionalMapping {
		std::string_view SourceFile;
		std::vector<TileMapping> TileMap;
	};
	struct BigMonsterSheet {
		std::string_view TargetFile;
		std::vector<std::string_view> SourceFiles;
		std::vector<AdditionalMapping> AdditionalTileMaps;
		SheetSize TargetSize{ .Width{ 2048 }, .Height{ 2048 } };
	};
	std::vector<BigMonsterSheet> sheets{
		{ "Data/Textures/monstersbig01.png", {
				"BigMonsters/quill_back.png",
				"BigMonsters/giant_spider.png",
				"BigMonsters/queen_bee.png",
			}, {
				{ "BigMonsters/giant_spider.png", {
						TileMapping{
							.SourceTile{ 0, 0, 256, 256 },
							.TargetTile{ 1792, 1280, 2048, 1536 }
						}
					}
				}
			}
		},
		{ "Data/Textures/monstersbig02.png", {
				"BigMonsters/mummy.png",
			}
		},
		{ "Data/Textures/monstersbig03.png", {
				"BigMonsters/lamassu.png",
				"BigMonsters/yeti_king.png",
				"BigMonsters/yeti_queen.png",
			}
		},
		{ "Data/Textures/monstersbig04.png", {
				"BigMonsters/crab_man.png",
				"BigMonsters/lavamander.png",
				"BigMonsters/giant_fly.png",
				"BigMonsters/giant_clam.png",
			}, {
				{ "BigMonsters/crab_man.png", {
						TileMapping{
							.SourceTile{ 0, 0, 256, 256 },
							.TargetTile{ 512, 512, 768, 768 }
						},
						TileMapping{
							.SourceTile{ 256, 0, 512, 256 },
							.TargetTile{ 1792, 256, 2048, 512 }
						}
					}
				},
				{ "BigMonsters/lavamander.png", {
						TileMapping{
							.SourceTile{ 0, 0, 768, 256 },
							.TargetTile{ 1280, 1280, 2048, 1536 }
						}
					}
				}
			}
		},
		{ "Data/Textures/monstersbig05.png", {
				"BigMonsters/ammit.png",
				"BigMonsters/madame_tusk.png",
				"BigMonsters/eggplant_minister.png",
				"BigMonsters/giant_frog.png",
			}, {
				{ "BigMonsters/eggplant_minister.png", {
						TileMapping{
							.SourceTile{ 0, 0, 512, 128 },
							.TargetTile{ 1152, 896, 1664, 1024 }
						},
						TileMapping{
							.SourceTile{ 0, 128, 384, 256 },
							.TargetTile{ 1664, 896, 2048, 1024 }
						}
					}
				}
			}
		},
		{ "Data/Textures/monstersbig06.png", {
				"BigMonsters/waddler.png",
				"BigMonsters/giant_fish.png",
			}
		},
		{ "Data/Textures/monsters_osiris.png", {
				"BigMonsters/alien_queen.png",
				"BigMonsters/osiris.png",
			}
		},
		{ "Data/Textures/monsters_ghost.png", {
				"Ghost/ghost_small_surprised.png",
				"Ghost/ghist.png",
				"Ghost/ghost.png",
				"Ghost/ghost_happy.png",
				"Ghost/ghost_sad.png",
				"Ghost/ghost_small_angry.png",
				"Ghost/ghost_small_happy.png",
				"Ghost/ghost_small_sad.png",
			}, {
				{ "Ghost/ghist.png", {
						TileMapping{
							.SourceTile{ 0, 0, 384, 128 },
							.TargetTile{ 896, 1408, 1280, 1536 }
						},
						TileMapping{
							.SourceTile{ 0, 128, 384, 258 },
							.TargetTile{ 1280, 1408, 1664, 1536 }
						},
						TileMapping{
							.SourceTile{ 0, 256, 128, 384 },
							.TargetTile{ 1664, 1408, 1792, 1536 }
						}
					}
				},
			}
		},
	};

	for (const auto& sheet : sheets) {
		std::vector<SourceSheet> source_sheets;
		for (std::string_view source_file : sheet.SourceFiles) {
			if (auto mapping = m_EntityDataExtractor->GetEntitySourceSheet(fmt::format("Data/Textures/Entities/{}", source_file))) {
				if (auto additional_mapping = algo::find(sheet.AdditionalTileMaps, &AdditionalMapping::SourceFile, source_file)) {
					mapping->TileMap.insert(mapping->TileMap.end(), additional_mapping->TileMap.begin(), additional_mapping->TileMap.end());
				}
				source_sheets.push_back(std::move(mapping).value());
			}
		}
		m_TargetSheets.push_back(TargetSheet{
				.Path{ sheet.TargetFile },
				.Size{ sheet.TargetSize },
				.SourceSheets{ std::move(source_sheets) },
				.RandomSelect{ false }
			});
	}
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
