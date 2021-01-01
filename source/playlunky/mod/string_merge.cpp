#include "string_merge.h"

#include "log.h"
#include "virtual_filesystem.h"
#include "util/algorithms.h"

#include <fmt/format.h>
#include <charconv>
#include <fstream>
#include <unordered_map>

bool StringMerger::RegisterOutdatedStringTable(std::string_view table) {
	std::uint8_t string_table;
	auto result = std::from_chars(table.data(), table.data() + table.size(), string_table);
	if (result.ec == std::errc::invalid_argument) {
		return false;
	}
	if (string_table > 8) {
		return false;
	}
	if (!algo::contains_if(mOutdatedStringTables, [string_table](const OutdatedStringTable& table) { return table.Index == string_table; })) {
		mOutdatedStringTables.push_back(OutdatedStringTable{
			.Index{ string_table },
			.Modded{ false }
		});
	}
	mNeedsRegen = true;
	return true;
}
bool StringMerger::RegisterModdedStringTable(std::string_view table) {
	std::uint8_t string_table;
	auto result = std::from_chars(table.data(), table.data() + table.size(), string_table);
	if (result.ec == std::errc::invalid_argument) {
		return false;
	}
	if (string_table > 8) {
		return false;
	}
	if (auto* existing_table = algo::find_if(mOutdatedStringTables, [string_table](const OutdatedStringTable& table) { return table.Index == string_table; })) {
		existing_table->Modded = true;
	}
	else {
		mOutdatedStringTables.push_back(OutdatedStringTable{
			.Index{ string_table },
			.Modded{ true }
			});
	}
	return true;
}

bool StringMerger::MergeStrings(
	const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder,
	const std::filesystem::path& hash_file_path, VirtualFilesystem& vfs) {

	namespace fs = std::filesystem;

	if (auto hash_file = std::ifstream{ source_folder / hash_file_path }) {
		{
			std::vector<std::uint8_t> all_string_tables{ 0, 1, 2, 3, 4, 5, 6, 7, 8 };

			for (auto outdated_string_table : mOutdatedStringTables) {
				if (outdated_string_table.Modded) {
					algo::erase(all_string_tables, outdated_string_table.Index);
				}
			}

			for (auto string_table : all_string_tables) {
				const auto string_table_name = fmt::format("strings{:02}.str", string_table);
				const auto string_table_destination_file = destination_folder / string_table_name;
				if (!fs::exists(string_table_destination_file)) {
					const auto string_table_source_file = vfs.GetFilePath(string_table_name).value_or(source_folder / string_table_name);
					if (!fs::exists(string_table_source_file)) {
						return false;
					}
					if (!fs::exists(destination_folder)) {
						fs::create_directories(destination_folder);
					}
					fs::copy_file(string_table_source_file, string_table_destination_file, fs::copy_options::overwrite_existing);
				}
			}
		}

		for (auto outdated_string_table : mOutdatedStringTables) {
			if (outdated_string_table.Modded) {
				struct ModdedString {
					std::string Hash;
					std::string String;
				};
				std::vector<ModdedString> modded_strings;

				{
					const auto string_table_mod_name = fmt::format("strings{:02}_mod.str", outdated_string_table.Index);
					const auto string_table_source_files = vfs.GetAllFilePaths(string_table_mod_name);

					for (const auto& string_table_source_file : string_table_source_files) {
						if (auto source_file = std::ifstream{ string_table_source_file }) {
							while (!source_file.eof()) {
								std::string modded_string;
								std::getline(source_file, modded_string);
								if (modded_string.size() >= 2 && modded_string[0] == '0' && modded_string[1] == 'x') {
									const auto colon_pos = modded_string.find(':');
									if (colon_pos != std::string::npos) {
										std::string_view hash_string = std::string_view{ modded_string }.substr(2, colon_pos - 2);
										std::string_view full_hash_string = std::string_view{ modded_string }.substr(0, colon_pos);

										std::uint32_t hash;
										auto result = std::from_chars(hash_string.data(), hash_string.data() + hash_string.size(), hash, 16);
										if (result.ec == std::errc::invalid_argument) {
											LogInfo("Failed parsing string hash '0x{}', modded string '{}' will be discarded...", hash_string, modded_string);
											return false;
										}

										if (!algo::contains_if(modded_strings,
											[full_hash_string](const ModdedString& modded_string) {
												return modded_string.Hash == full_hash_string;
											})) {

											const std::size_t string_start = 2 + hash_string.size();
											std::string_view string = std::string_view{ modded_string }.substr(modded_string.find_first_not_of(' ', string_start + 1));
											modded_strings.push_back(ModdedString{
												.Hash{ std::string{ full_hash_string } },
												.String{ std::string{ string } }
											});
										}
									}
									else {
										LogInfo("Failed parsing modded string '{}', expected ':' after hash, the string will be discarded...", modded_string);
									}
								}
								else if (modded_string.size() > 0 && modded_string[0] != '#') {
									LogInfo("Failed parsing modded string '{}', expected hash at beginning of line, the string will be discarded...", modded_string);
								}
							}
						}
					}
				}

				const auto string_table_name = fmt::format("strings{:02}.str", outdated_string_table.Index);
				const auto string_table_source_file = source_folder / string_table_name;
				const auto string_table_destination_file = destination_folder / string_table_name;

				auto strings_source_file = std::ifstream{ string_table_source_file };
				auto strings_destination_file = std::ofstream{ string_table_destination_file, std::ios::trunc };

				if (strings_source_file && strings_destination_file) {
					hash_file.seekg(0, std::ios::beg);

					while (!strings_source_file.eof() && !hash_file.eof()) {
						std::string source_string;
						std::getline(strings_source_file, source_string);

						std::string hash_string;
						std::getline(hash_file, hash_string);
						
						if (hash_string != "0xdeadbeef") {
							if (auto* modded_string = algo::find_if(modded_strings,
								[&hash_string](const ModdedString& modded_string) {
									return modded_string.Hash == hash_string;
								}))
							{
								strings_destination_file << modded_string->String << '\n';
								continue;
							}
						}

						strings_destination_file << source_string << '\n';
					}
				}

			}
		}
		return true;
	}

	return false;
}