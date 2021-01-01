#include "string_hash.h"

#include "util/algorithms.h"
#include "util/regex.h"

#include <fstream>
#include <span>

static constexpr ctll::fixed_string s_CommentBlockRule{ "#+" };
static constexpr ctll::fixed_string s_CommentBlockNameRule{ "#+\\s*([^#]+)\\s*#*" };

std::uint32_t HashString(std::string_view string) {
	return static_cast<std::uint32_t>(std::hash<std::string_view>{}(string) & 0xFFFFFFFF);
}

bool CreateHashedStringsFile(const std::filesystem::path& source_file, const std::filesystem::path& destination_file) {
	namespace fs = std::filesystem;
	{
		const auto destination_parent_folder = destination_file.parent_path();
		if (!fs::exists(destination_parent_folder)) {
			fs::create_directories(destination_parent_folder);
		}
	}

	if (auto source = std::ifstream{ source_file }) {
		if (auto destination = std::ofstream{ destination_file, std::ios::trunc }) {
			destination << std::hex << std::setfill('0');

			std::string current_comment_block;
			while (!source.eof()) {
				std::string line;
				std::getline(source, line);
				if (ctre::starts_with<s_CommentBlockRule>(line)) {
					if (const auto name_match = ctre::match<s_CommentBlockNameRule>(line)) {
						current_comment_block = algo::trim(name_match.get<1>().to_string());
					}
					destination << "0x" << std::setw(8) << 0xdeadbeef << '\n';
				}
				else {
					const auto comment_and_line = line + current_comment_block;
					destination << "0x" << std::setw(8) << HashString(comment_and_line) << '\n';
				}
			}
		}
	}
	
	return true;
}
