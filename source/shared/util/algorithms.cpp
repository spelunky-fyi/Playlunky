#include "algorithms.h"

#include <cctype>

namespace algo {
	bool is_sub_path(const std::filesystem::path& path, const std::filesystem::path& base) {
		const auto first_mismatch = std::mismatch(path.begin(), path.end(), base.begin(), base.end());
		return first_mismatch.second == base.end();
	}
	bool is_end_of_path(const std::filesystem::path& path, const std::filesystem::path& base) {
		const auto begin_of_sub_path = std::search(base.begin(), base.end(), path.begin(), path.end());
		return std::distance(begin_of_sub_path, base.end()) == std::distance(path.begin(), path.end());
	}

	std::string trim(std::string str) {
		str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
			return !std::isspace(ch);
		}));
		str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
		}).base(), str.end());
		return std::move(str);
	}
	std::string trim(std::string str, char to_strip) {
		str.erase(str.begin(), std::find_if(str.begin(), str.end(), [to_strip](unsigned char ch) {
			return ch != to_strip;
		}));
		str.erase(std::find_if(str.rbegin(), str.rend(), [to_strip](unsigned char ch) {
			return ch != to_strip;
		}).base(), str.end());
		return std::move(str);
	}

	bool case_insensitive_equal(std::string_view lhs, std::string_view rhs) {
		struct case_insensitive_char_traits : public std::char_traits<char> {
			static bool eq(char c1, char c2) { return toupper(c1) == toupper(c2); }
			static bool ne(char c1, char c2) { return toupper(c1) != toupper(c2); }
			static bool lt(char c1, char c2) { return toupper(c1) < toupper(c2); }
			static int compare(const char* s1, const char* s2, size_t n) {
				while (n-- != 0) {
					if (toupper(*s1) < toupper(*s2)) return -1;
					if (toupper(*s1) > toupper(*s2)) return 1;
					++s1; ++s2;
				}
				return 0;
			}
			static const char* find(const char* s, int n, char a) {
				while (n-- > 0 && toupper(*s) != toupper(a)) {
					++s;
				}
				return s;
			}
		};
		using case_insensitive_string_view = std::basic_string_view<char, case_insensitive_char_traits>;
		return case_insensitive_string_view{ lhs.data(), lhs.size() } == case_insensitive_string_view{ rhs.data(), rhs.size() };
	}
}
