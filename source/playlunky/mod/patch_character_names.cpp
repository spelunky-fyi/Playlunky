#include "patch_character_names.h"

#include "log.h"
#include "virtual_filesystem.h"
#include "detour/sigfun.h"
#include "detour/sigscan.h"
#include "util/algorithms.h"
#include "util/format.h"

#include <array>
#include <cassert>
#include <charconv>
#include <codecvt>
#include <fstream>
#include <unordered_map>
#include <span>

struct Color {
	float r;
	float g;
	float b;
	float a;
};
struct CharacterDefinition {
	float _float_0;
	float _float_1;
	float _float_2;
	float _float_3;
	float _float_4;
	Color Color;
	char16_t FullName[0x18];
	char16_t ShortName[0xc];
};
static constexpr auto sizeof_CharacterDefinition = sizeof(CharacterDefinition);
static constexpr auto expect_CharacterDefinition = 0x36 * sizeof(char16_t);
static_assert(sizeof_CharacterDefinition == expect_CharacterDefinition);

std::span<CharacterDefinition> GetCharacterDefinitions() {
	const auto static_init_strings = SigScan::FindPattern("Spel2.exe", "\x48\x8b\xc4\x48\x81\xec\xe8\x00\x00\x00\x0f\x29\x70\xe8\x0f\x29\x78\xd8\x44\x0f\x29\x40\xc8\x44\x0f\x29\x48\xb8\x44\x0f\x29\x50\xa8"_sig, true);
	const auto load_ana_strings = SigScan::FindPattern("\xf3\x44\x0f\x10\xc8"_sig, static_init_strings, (void*)((const char*)static_init_strings + 0x1000));
	// load_ana_strings = (char*)load_ana_strings + 12;
	const auto write_ana_strings = SigScan::FindPattern("\x0f\x11"_sig, load_ana_strings, (void*)((const char*)static_init_strings + 0x1000));

	auto decode_pc = [](void* instruction_addr) {
		off_t rel = *(int32_t*)((char*)instruction_addr + 3);
		return (char*)instruction_addr + rel + 7;
	};

	auto ana_strings = decode_pc(write_ana_strings);
	auto string_table_first_element = ana_strings + sizeof(CharacterDefinition::FullName) + sizeof(CharacterDefinition::ShortName);
	auto string_table_start = string_table_first_element - sizeof(CharacterDefinition);
	std::span<CharacterDefinition> character_table{ (CharacterDefinition*)(string_table_start), 20 };
	[[maybe_unused]] constexpr std::array known_tables{
		CharacterDefinition{ .FullName{ u"Ana Spelunky" }, .ShortName{ u"Ana" } },
		CharacterDefinition{ .FullName{ u"Margaret Tunnel" }, .ShortName{ u"Margaret" } },
		CharacterDefinition{ .FullName{ u"Colin Northward" }, .ShortName{ u"Colin" } },
		CharacterDefinition{ .FullName{ u"Roffy D. Sloth" }, .ShortName{ u"Roffy" } },
		CharacterDefinition{ .FullName{ u"Alto Singh" }, .ShortName{ u"Alto" } },
		CharacterDefinition{ .FullName{ u"Liz Mutton" }, .ShortName{ u"Liz" } },
		CharacterDefinition{ .FullName{ u"Nekka The Eagle" }, .ShortName{ u"Nekka" } },
		CharacterDefinition{ .FullName{ u"LISE Project" }, .ShortName{ u"LISE" } },
		CharacterDefinition{ .FullName{ u"Coco Von Diamonds" }, .ShortName{ u"Coco" } },
		CharacterDefinition{ .FullName{ u"Manfred Tunnel" }, .ShortName{ u"Manfred" } },
		CharacterDefinition{ .FullName{ u"Little Jay" }, .ShortName{ u"Jay" } },
		CharacterDefinition{ .FullName{ u"Tina Flan" }, .ShortName{ u"Tina" } },
		CharacterDefinition{ .FullName{ u"Valerie Crump" }, .ShortName{ u"Valerie" } },
		CharacterDefinition{ .FullName{ u"Au" }, .ShortName{ u"Au" } },
		CharacterDefinition{ .FullName{ u"Demi Von Diamonds" }, .ShortName{ u"Demi" } },
		CharacterDefinition{ .FullName{ u"Pilot" }, .ShortName{ u"Pilot" } },
		CharacterDefinition{ .FullName{ u"Princess Airyn" }, .ShortName{ u"Airyn" } },
		CharacterDefinition{ .FullName{ u"Dirk Yamaoka" }, .ShortName{ u"Dirk" } },
		CharacterDefinition{ .FullName{ u"Guy Spelunky" }, .ShortName{ u"Guy" } },
		CharacterDefinition{ .FullName{ u"Classic Guy" }, .ShortName{ u"Classic Guy" } },
	};
	for (size_t i = 0; i < 20; i++) {
		[[maybe_unused]]CharacterDefinition& string_table = character_table[i];
		[[maybe_unused]] const CharacterDefinition& known_table = known_tables[i];
		assert(memcmp(string_table.FullName, known_table.FullName, sizeof(CharacterDefinition::FullName)) == 0);
		assert(memcmp(string_table.ShortName, known_table.ShortName, sizeof(CharacterDefinition::ShortName)) == 0);
	}

	return character_table;
}

template<typename T>
std::string toUTF8(const std::basic_string<T, std::char_traits<T>>& source)
{
	std::wstring_convert<std::codecvt_utf8_utf16<T>, T> convertor;
	return convertor.to_bytes(source);
}

template<typename T>
std::basic_string<T, std::char_traits<T>, std::allocator<T>> fromUTF8(const std::string& source)
{
	std::wstring_convert<std::codecvt_utf8_utf16<T>, T> convertor;
	return convertor.from_bytes(source);
}

void PatchCharacterDefinitions(VirtualFilesystem& vfs) {
	std::span<CharacterDefinition> character_table = GetCharacterDefinitions();

	constexpr std::string_view characters[]{
		"yellow",
		"magenta",
		"cyan",
		"black",
		"cinnabar",
		"green",
		"olive",
		"white",
		"cerulean",
		"blue",
		"lime",
		"lemon",
		"iris",
		"gold",
		"red",
		"pink",
		"violet",
		"gray",
		"khaki",
		"orange"
	};
	for (size_t i = 0; i < sizeof(characters) / sizeof(std::string_view); i++) {
		auto char_name = characters[i];

		auto name_file_name = fmt::format("char_{}.name", char_name);
		if (auto file_path = vfs.GetFilePath(name_file_name)) {
			if (auto strings_source_file = std::ifstream{ file_path.value() }) {
				std::string full_name_narrow;
				std::string short_name_narrow;

				std::getline(strings_source_file, full_name_narrow);
				if (!strings_source_file.eof()) {
					std::getline(strings_source_file, short_name_narrow);
				}

				{
					full_name_narrow = algo::trim(full_name_narrow);
					const char full_name_str[]{ "FullName:" };
					const auto colon_pos = full_name_narrow.find(full_name_str);
					if (colon_pos == 0) {
						full_name_narrow = algo::trim(full_name_narrow.substr(sizeof(full_name_str) - 1));
					}
				}

				if (short_name_narrow.empty()) {
					short_name_narrow = full_name_narrow;
					const auto space = short_name_narrow.find(' ');
					if (space != std::string::npos) {
						short_name_narrow = algo::trim(short_name_narrow.substr(0, space + 1));
					}
				}
				else {
					short_name_narrow = algo::trim(short_name_narrow);
					const char short_name_str[]{ "ShortName:" };
					const auto colon_pos = short_name_narrow.find(short_name_str);
					if (colon_pos != std::string::npos) {
						short_name_narrow = algo::trim(short_name_narrow.substr(sizeof(short_name_str) - 1));
					}
				}

				CharacterDefinition& character_def = character_table[i];

				{
					std::u16string full_name = fromUTF8<char16_t>(full_name_narrow);
					const auto max_size = sizeof(CharacterDefinition::FullName) / sizeof(char16_t);
					if (full_name.size() > max_size) {
						LogError("Character name {} is too long, max supported size is {}", char_name, max_size);
					}
					memset(character_def.FullName, 0, max_size * sizeof(char16_t));
					memcpy(character_def.FullName, full_name.c_str(), std::min(full_name.size(), max_size) * sizeof(char16_t));
				}

				{
					std::u16string short_name = fromUTF8<char16_t>(short_name_narrow);
					const auto max_size = sizeof(CharacterDefinition::ShortName) / sizeof(char16_t);
					if (short_name.size() > max_size) {
						LogError("Character name {} is too long, max supported size is {}", char_name, max_size);
					}
					memset(character_def.ShortName, 0, max_size * sizeof(char16_t));
					memcpy(character_def.ShortName, short_name.c_str(), std::min(short_name.size(), max_size) * sizeof(char16_t));
				}
			}
		}

		auto color_file_name = fmt::format("char_{}.color", char_name);
		if (auto file_path = vfs.GetFilePath(color_file_name)) {
			if (auto character_source_file = std::ifstream{ file_path.value() }) {
				CharacterDefinition& character_def = character_table[i];
				try {
					float r, g, b;
					character_source_file >> r >> g >> b;
					character_def.Color.r = r;
					character_def.Color.g = g;
					character_def.Color.b = b;
				}
				catch (...) {
					LogError("Character color file '{}' has bad formatting, expected `float float float`", file_path.value().string());
				}

				if (!character_source_file.eof()) {
					character_source_file >> character_def.Color.a;
				}
			}
		}
	}
}
