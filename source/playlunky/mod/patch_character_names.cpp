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

//struct StringTable {
//	char16_t FullName[0x18];
//	char16_t ShortName[0xc];
//	char16_t Other[0x12];
//};
struct StringTable {
	char16_t FullName[0x18];
	char16_t ShortName[0xc];
	float _float_0;
	float _float_1;
	float _float_2;
	float _float_3;
	float _float_4;
	float _float_5;
	float _float_7;
	float _float_8;
	float _float_9;
};
static constexpr auto sizeof_StringTable = sizeof(StringTable);
static constexpr auto expect_StringTable = 0x36 * sizeof(char16_t);
static_assert(sizeof_StringTable == expect_StringTable);

std::span<StringTable> GetStringTables() {
	const auto static_init_strings = SigScan::FindPattern("Spel2.exe", "\x48\x8b\xc4\x48\x81\xec\xe8\x00\x00\x00\x0f\x29\x70\xe8\x0f\x29\x78\xd8\x44\x0f\x29\x40\xc8\x44\x0f\x29\x48\xb8\x44\x0f\x29\x50\xa8"_sig, true);
	const auto load_ana_strings = SigScan::FindPattern("\xf3\x44\x0f\x10\xc8"_sig, static_init_strings, (void*)((const char*)static_init_strings + 0x1000));
	// load_ana_strings = (char*)load_ana_strings + 12;
	const auto write_ana_strings = SigScan::FindPattern("\x0f\x11"_sig, load_ana_strings, (void*)((const char*)static_init_strings + 0x1000));

	auto decode_pc = [](void* instruction_addr) {
		off_t rel = *(int32_t*)((char*)instruction_addr + 3);
		return (char*)instruction_addr + rel + 7;
	};

	std::span<StringTable> string_tables{ (StringTable*)decode_pc(write_ana_strings), 20 };
	[[maybe_unused]] constexpr std::array known_tables{
		StringTable{ .FullName{ u"Ana Spelunky" }, .ShortName{ u"Ana" } },
		StringTable{ .FullName{ u"Margaret Tunnel" }, .ShortName{ u"Margaret" } },
		StringTable{ .FullName{ u"Colin Northward" }, .ShortName{ u"Colin" } },
		StringTable{ .FullName{ u"Roffy D. Sloth" }, .ShortName{ u"Roffy" } },
		StringTable{ .FullName{ u"Alto Singh" }, .ShortName{ u"Alto" } },
		StringTable{ .FullName{ u"Liz Mutton" }, .ShortName{ u"Liz" } },
		StringTable{ .FullName{ u"Nekka The Eagle" }, .ShortName{ u"Nekka" } },
		StringTable{ .FullName{ u"LISE Project" }, .ShortName{ u"LISE" } },
		StringTable{ .FullName{ u"Coco Von Diamonds" }, .ShortName{ u"Coco" } },
		StringTable{ .FullName{ u"Manfred Tunnel" }, .ShortName{ u"Manfred" } },
		StringTable{ .FullName{ u"Little Jay" }, .ShortName{ u"Jay" } },
		StringTable{ .FullName{ u"Tina Flan" }, .ShortName{ u"Tina" } },
		StringTable{ .FullName{ u"Valerie Crump" }, .ShortName{ u"Valerie" } },
		StringTable{ .FullName{ u"Au" }, .ShortName{ u"Au" } },
		StringTable{ .FullName{ u"Demi Von Diamonds" }, .ShortName{ u"Demi" } },
		StringTable{ .FullName{ u"Pilot" }, .ShortName{ u"Pilot" } },
		StringTable{ .FullName{ u"Princess Airyn" }, .ShortName{ u"Airyn" } },
		StringTable{ .FullName{ u"Dirk Yamaoka" }, .ShortName{ u"Dirk" } },
		StringTable{ .FullName{ u"Guy Spelunky" }, .ShortName{ u"Guy" } },
		StringTable{ .FullName{ u"Classic Guy" }, .ShortName{ u"Classic Guy" } },
	};
	for (size_t i = 0; i < 20; i++) {
		[[maybe_unused]]StringTable& string_table = string_tables[i];
		[[maybe_unused]] const StringTable& known_table = known_tables[i];
		assert(memcmp(string_table.FullName, known_table.FullName, sizeof(StringTable::FullName)) == 0);
		assert(memcmp(string_table.ShortName, known_table.ShortName, sizeof(StringTable::ShortName)) == 0);
	}

	return string_tables;
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

void PatchCharacterNames(VirtualFilesystem& vfs) {
	std::span<StringTable> string_tables = GetStringTables();

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

		auto file_name = fmt::format("char_{}.name", char_name);
		if (auto file_path = vfs.GetFilePath(file_name)) {
			auto strings_source_file = std::ifstream{ file_path.value() };
			if (!strings_source_file.eof()) {
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

				StringTable& string_table = string_tables[i];

				{
					std::u16string full_name = fromUTF8<char16_t>(full_name_narrow);
					const auto max_size = sizeof(StringTable::FullName) / sizeof(char16_t);
					if (full_name.size() > max_size) {
						LogError("Character name {} is too long, max supported size is {}", char_name, max_size);
					}
					memset(string_table.FullName, 0, max_size * sizeof(char16_t));
					memcpy(string_table.FullName, full_name.c_str(), std::min(full_name.size(), max_size) * sizeof(char16_t));
				}

				{
					std::u16string short_name = fromUTF8<char16_t>(short_name_narrow);
					const auto max_size = sizeof(StringTable::ShortName) / sizeof(char16_t);
					if (short_name.size() > max_size) {
						LogError("Character name {} is too long, max supported size is {}", char_name, max_size);
					}
					memset(string_table.ShortName, 0, max_size * sizeof(char16_t));
					memcpy(string_table.ShortName, short_name.c_str(), std::min(short_name.size(), max_size) * sizeof(char16_t));
				}
			}
		}
	}
}
