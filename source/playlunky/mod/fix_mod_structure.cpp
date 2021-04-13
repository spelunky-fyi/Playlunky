#include "unzip_mod.h"

#include "log.h"
#include "util/algorithms.h"
#include "util/regex.h"

static constexpr ctll::fixed_string s_FontRule{ ".*\\.fnb" };
static constexpr std::string_view s_FontTargetPath{ "Data/Fonts" };

static constexpr ctll::fixed_string s_ArenaLevelRule{ "dm.*\\.lvl" };
static constexpr ctll::fixed_string s_ArenaLevelTokRule{ ".*\\.tok" };
static constexpr std::string_view s_ArenaLevelTargetPath{ "Data/Levels/Arena" };

static constexpr ctll::fixed_string s_LevelRule{ ".*\\.lvl" };
static constexpr std::string_view s_LevelTargetPath{ "Data/Levels" };

static constexpr ctll::fixed_string s_OldTextureRule{ "ai\\.(DDS|png)" };
static constexpr std::string_view s_OldTextureTargetPath{ "Data/Textures/OldTextures" };

static constexpr ctll::fixed_string s_FullTextureRule{ ".*_full\\.(DDS|png)" };
static constexpr std::string_view s_FullTextureTargetPath{ "Data/Textures/Entities" };

static constexpr ctll::fixed_string s_TextureRule{ ".*\\.(DDS|png)" };
static constexpr std::string_view s_TextureTargetPath{ "Data/Textures" };

static constexpr ctll::fixed_string s_WavRule{ ".*\\.wav" };
static constexpr std::string_view s_WavTargetPath{ "soundbank/wav" };

static constexpr ctll::fixed_string s_OggRule{ ".*\\.ogg" };
static constexpr std::string_view s_OggTargetPath{ "soundbank/ogg" };

static constexpr ctll::fixed_string s_Mp3Rule{ ".*\\.mp3" };
static constexpr std::string_view s_Mp3TargetPath{ "soundbank/mp3" };

static constexpr ctll::fixed_string s_WvRule{ ".*\\.wv" };
static constexpr std::string_view s_WvTargetPath{ "soundbank/wv" };

static constexpr ctll::fixed_string s_OpusRule{ ".*\\.opus" };
static constexpr std::string_view s_OpusTargetPath{ "soundbank/opus" };

static constexpr ctll::fixed_string s_FlacRule{ ".*\\.flac" };
static constexpr std::string_view s_FlacTargetPath{ "soundbank/flac" };

static constexpr ctll::fixed_string s_MpcRule{ ".*\\.mpc" };
static constexpr std::string_view s_MpcTargetPath{ "soundbank/mpc" };

static constexpr ctll::fixed_string s_MppRule{ ".*\\.mpp" };
static constexpr std::string_view s_MppTargetPath{ "soundbank/mpp" };

static constexpr std::string_view s_PetsEntityFiles[]{
	"monty",
	"percy",
	"poochi",
};
static constexpr std::string_view s_MountsEntityFiles[]{
	"turkey",
	"rockdog",
	"axolotl",
	"qilin",
};
static constexpr std::string_view s_GhostEntityFiles[]{
	"ghist",
	"ghost",
	"ghost_happy",
	"ghost_sad",
	"ghost_small_angry",
	"ghost_small_happy",
	"ghost_small_sad",
	"ghost_small_surprised",
};
static constexpr std::string_view s_CrittersEntityFiles[]{
	"anchovy",
	"butterfly",
	"crab",
	"blue_crab",
	"dung_beetle",
	"firefly",
	"fish",
	"locust",
	"slime",
	"snail",
	"drone",
	"penguin",
	"birdies"
};
static constexpr std::string_view s_MonstersEntityFiles[]{
	"alien",
	"bat",
	"bee",
	"cat_mummy",
	"cave_man",
	"cobra",
	"croc_man",
	"female_jiangshi",
	"fire_bug",
	"fire_frog",
	"fly",
	"flying_fish",
	"frog",
	"golden_monkey",
	"grub",
	"hang_spider",
	"hermit_crab",
	"horned_lizard",
	"imp",
	"jiangshi",
	"jumpdog",
	"leprechaun",
	"magmar",
	"man_trap",
	"mole",
	"monkey",
	"mosquito",
	"necromancer",
	"octopus",
	"olmite_armored",
	"olmite_helmet",
	"olmite_naked",
	"proto_shopkeeper",
	"robot",
	"scorpion",
	"skeleton",
	"snake",
	"sorceress",
	"spider",
	"tiki_man",
	"ufo",
	"vampire",
	"vlad",
	"witch_doctor",
	"witch_doctor_skull",
	"yeti",
	"tadpole"
};
static constexpr std::string_view s_BigMonstersEntityFiles[]{
	"alien_queen",
	"ammit",
	"crab_man",
	"eggplant_minister",
	"giant_clam",
	"giant_fish",
	"giant_fly",
	"giant_frog",
	"giant_spider",
	"lamassu",
	"lavamander",
	"madame_tusk",
	"mummy",
	"osiris",
	"queen_bee",
	"quill_back",
	"waddler",
	"yeti_king",
	"yeti_queen",
};
static constexpr std::string_view s_PeopleEntityFiles[]{
	"bodyguard",
	"hunduns_servant",
	"merchant",
	"old_hunter",
	"parmesan",
	"parsley",
	"parsnip",
	"shopkeeper",
	"thief",
	"yang",
};
static constexpr std::string_view s_KnownTextureFiles[]{
	"base_eggship",
	"base_eggship2",
	"base_eggship3",
	"base_skynight",
	"base_surface",
	"base_surface2",
	"bayer8",
	"bg_babylon",
	"bg_beehive",
	"bg_cave",
	"bg_duat",
	"bg_duat2",
	"bg_eggplant",
	"bg_gold",
	"bg_ice",
	"bg_jungle",
	"bg_mothership",
	"bg_stone",
	"bg_sunken",
	"bg_temple",
	"bg_tidepool",
	"bg_vlad",
	"bg_volcano",
	"border_main",
	"char_black",
	"char_blue",
	"char_cerulean",
	"char_cinnabar",
	"char_cyan",
	"char_eggchild",
	"char_gold",
	"char_gray",
	"char_green",
	"char_hired",
	"char_iris",
	"char_khaki",
	"char_lemon",
	"char_lime",
	"char_magenta",
	"char_olive",
	"char_orange",
	"char_pink",
	"char_red",
	"char_violet",
	"char_white",
	"char_yellow",
	"coffins",
	"credits",
	"deco_babylon",
	"deco_basecamp",
	"deco_cave",
	"deco_cosmic",
	"deco_eggplant",
	"deco_extra",
	"deco_gold",
	"deco_ice",
	"deco_jungle",
	"deco_sunken",
	"deco_temple",
	"deco_tidepool",
	"deco_tutorial",
	"deco_volcano",
	"floor_babylon",
	"floor_cave",
	"floor_eggplant",
	"floor_ice",
	"floor_jungle",
	"floor_sunken",
	"floor_surface",
	"floor_temple",
	"floor_tidepool",
	"floor_volcano",
	"floormisc",
	"floorstyled_babylon",
	"floorstyled_beehive",
	"floorstyled_duat",
	"floorstyled_gold_normal",
	"floorstyled_gold",
	"floorstyled_guts",
	"floorstyled_mothership",
	"floorstyled_pagoda",
	"floorstyled_palace",
	"floorstyled_stone",
	"floorstyled_sunken",
	"floorstyled_temple",
	"floorstyled_vlad",
	"floorstyled_wood",
	"fontdebug",
	"fontfirasans",
	"fontmono",
	"fontnewrodin",
	"fontrodincattleya",
	"fontyorkten",
	"fx_ankh",
	"fx_big",
	"fx_explosion",
	"fx_rubble",
	"fx_small",
	"fx_small2",
	"fx_small3",
	"hud_controller_buttons",
	"hud_text",
	"hud",
	"items_ushabti",
	"items",
	"journal_back",
	"journal_elements",
	"journal_entry_bg",
	"journal_entry_items",
	"journal_entry_mons_big",
	"journal_entry_mons",
	"journal_entry_people",
	"journal_entry_place",
	"journal_entry_traps",
	"journal_pageflip",
	"journal_pagetorn",
	"journal_select",
	"journal_stickers",
	"journal_story",
	"journal_top_entry",
	"journal_top_gameover",
	"journal_top_main",
	"journal_top_profile",
	"loading",
	"lut_backlayer",
	"lut_blackmarket",
	"lut_icecaves",
	"lut_original",
	"lut_vlad",
	"main_body",
	"main_dirt",
	"main_door",
	"main_doorback",
	"main_doorframe",
	"main_fore1",
	"main_fore2",
	"main_head",
	"menu_basic",
	"menu_brick1",
	"menu_brick2",
	"menu_cave1",
	"menu_cave2",
	"menu_chardoor",
	"menu_charsel",
	"menu_deathmatch",
	"menu_deathmatch2",
	"menu_deathmatch3",
	"menu_deathmatch4",
	"menu_deathmatch5",
	"menu_deathmatch6",
	"menu_disp",
	"menu_generic",
	"menu_header",
	"menu_leader",
	"menu_online",
	"menu_title",
	"menu_titlegal",
	"menu_tunnel",
	"monsters_ghost",
	"monsters_hundun",
	"monsters_olmec",
	"monsters_osiris",
	"monsters_pets",
	"monsters_tiamat",
	"monsters_yama",
	"monsters01",
	"monsters02",
	"monsters03",
	"monstersbasic01",
	"monstersbasic02",
	"monstersbasic03",
	"monstersbig01",
	"monstersbig02",
	"monstersbig03",
	"monstersbig04",
	"monstersbig05",
	"monstersbig06",
	"mounts",
	"noise0",
	"noise1",
	"saving",
	"shadows",
	"shine",
	"splash0",
	"splash1",
	"splash2",
};
static constexpr std::string_view s_RestKnownFiles[]{
	"soundbank.strings.bank",
	"strings00.str",
	"strings00_mod.str",
	"strings00_hashed.str",
	"strings01.str",
	"strings01_mod.str",
	"strings01_hashed.str",
	"strings02.str",
	"strings02_mod.str",
	"strings02_hashed.str",
	"strings03.str",
	"strings03_mod.str",
	"strings03_hashed.str",
	"strings04.str",
	"strings04_mod.str",
	"strings04_hashed.str",
	"strings05.str",
	"strings05_mod.str",
	"strings05_hashed.str",
	"strings06.str",
	"strings06_mod.str",
	"strings06_hashed.str",
	"strings07.str",
	"strings07_mod.str",
	"strings07_hashed.str",
	"strings08.str",
	"strings08_mod.str",
	"strings08_hashed.str",
	"shaders.hlsl",
	"shaders_mod.hlsl",
	"soundbank.bank",
};

void FixModFolderStructure(const std::filesystem::path& mod_folder) {
	namespace fs = std::filesystem;
	struct PathMapping {
		fs::path CurrentPath;
		fs::path TargetPath;
	};
	std::vector<PathMapping> path_mappings;

	const auto db_folder = mod_folder / ".db";

	for (const auto& path : fs::recursive_directory_iterator(mod_folder)) {
		if (fs::is_regular_file(path) && !algo::is_sub_path(path, db_folder)) {
			const auto file_name = path.path().filename().string();
			const auto file_stem = path.path().stem().string();
			if (ctre::match<s_FontRule>(file_name)) {
				path_mappings.push_back({ path, mod_folder / s_FontTargetPath / file_name });
			}
			else if (ctre::match<s_ArenaLevelRule>(file_name) || ctre::match<s_ArenaLevelTokRule>(file_name)) {
				path_mappings.push_back({ path, mod_folder / s_ArenaLevelTargetPath / file_name });
			}
			else if (ctre::match<s_LevelRule>(file_name)) {
				path_mappings.push_back({ path, mod_folder / s_LevelTargetPath / file_name });
			}
			else if (ctre::match<s_TextureRule>(file_name)) {
				if (ctre::match<s_OldTextureRule>(file_name)) {
					path_mappings.push_back({ path, mod_folder / s_OldTextureTargetPath / file_name });
				}
				else if (ctre::match<s_FullTextureRule>(file_name)) {
					path_mappings.push_back({ path, mod_folder / s_FullTextureTargetPath / file_name });
				}
				else if (algo::contains(s_PetsEntityFiles, file_stem)) {
					path_mappings.push_back({ path, mod_folder / s_FullTextureTargetPath / "Pets" / file_name });
				}
				else if (algo::contains(s_MountsEntityFiles, file_stem)) {
					path_mappings.push_back({ path, mod_folder / s_FullTextureTargetPath / "Mounts" / file_name });
				}
				else if (algo::contains(s_GhostEntityFiles, file_stem)) {
					path_mappings.push_back({ path, mod_folder / s_FullTextureTargetPath / "Ghost" / file_name });
				}
				else if (algo::contains(s_MonstersEntityFiles, file_stem)) {
					path_mappings.push_back({ path, mod_folder / s_FullTextureTargetPath / "Monsters" / file_name });
				}
				else if (algo::contains(s_CrittersEntityFiles, file_stem)) {
					path_mappings.push_back({ path, mod_folder / s_FullTextureTargetPath / "Critters" / file_name });
				}
				else if (algo::contains(s_MonstersEntityFiles, file_stem)) {
					path_mappings.push_back({ path, mod_folder / s_FullTextureTargetPath / "Monsters" / file_name });
				}
				else if (algo::contains(s_BigMonstersEntityFiles, file_stem)) {
					path_mappings.push_back({ path, mod_folder / s_FullTextureTargetPath / "BigMonsters" / file_name });
				}
				else if (algo::contains(s_PeopleEntityFiles, file_stem)) {
					path_mappings.push_back({ path, mod_folder / s_FullTextureTargetPath / "People" / file_name });
				}
				else if (algo::contains(s_KnownTextureFiles, file_stem)) {
					path_mappings.push_back({ path, mod_folder / s_TextureTargetPath / file_name });
				}
			}
			else if (ctre::match<s_WavRule>(file_name)) {
				path_mappings.push_back({ path, mod_folder / s_WavTargetPath / file_name });
			}
			else if (ctre::match<s_OggRule>(file_name)) {
				path_mappings.push_back({ path, mod_folder / s_OggTargetPath / file_name });
			}
			else if (ctre::match<s_Mp3Rule>(file_name)) {
				path_mappings.push_back({ path, mod_folder / s_Mp3TargetPath / file_name });
			}
			else if (ctre::match<s_WvRule>(file_name)) {
				path_mappings.push_back({ path, mod_folder / s_WvTargetPath / file_name });
			}
			else if (ctre::match<s_OpusRule>(file_name)) {
				path_mappings.push_back({ path, mod_folder / s_OpusTargetPath / file_name });
			}
			else if (ctre::match<s_FlacRule>(file_name)) {
				path_mappings.push_back({ path, mod_folder / s_FlacTargetPath / file_name });
			}
			else if (ctre::match<s_MpcRule>(file_name)) {
				path_mappings.push_back({ path, mod_folder / s_MpcTargetPath / file_name });
			}
			else if (ctre::match<s_MppRule>(file_name)) {
				path_mappings.push_back({ path, mod_folder / s_MppTargetPath / file_name });
			}
			else if (algo::contains(s_RestKnownFiles, file_name)) {
				path_mappings.push_back({ path, mod_folder / file_name });
			}
		}
	}

	for (auto& [current_path, target_path] : path_mappings) {
		if (current_path != target_path) {
			{
				const auto target_parent_path = target_path.parent_path();
				if (!fs::exists(target_parent_path)) {
					fs::create_directories(target_parent_path);
				}
			}

			fs::rename(current_path, target_path);
		}
	}
}
