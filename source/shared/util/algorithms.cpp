#include "algorithms.h"

#include <cctype>
#include <codecvt>

namespace algo
{
std::string path_string(const std::filesystem::path& path)
{
    std::string str = path.string();
    std::replace(str.begin(), str.end(), '\\', '/');
    return str;
}
bool is_same_path(const std::filesystem::path& lhs, const std::filesystem::path& rhs)
{
    const auto lhs_str = path_string(lhs);
    const auto rhs_str = path_string(rhs);
    return case_insensitive_equal(lhs_str, rhs_str);
}
bool is_sub_path(const std::filesystem::path& path, const std::filesystem::path& base)
{
    const auto first_mismatch = std::mismatch(path.begin(), path.end(), base.begin(), base.end());
    return first_mismatch.second == base.end();
}
bool is_end_of_path(const std::filesystem::path& path, const std::filesystem::path& base)
{
    const auto begin_of_sub_path = std::search(base.begin(), base.end(), path.begin(), path.end());
    return std::distance(begin_of_sub_path, base.end()) == std::distance(path.begin(), path.end());
}
std::filesystem::path strip_end_of_path(const std::filesystem::path& path, const std::filesystem::path& base)
{
    std::filesystem::path ret{};
    {
        const auto begin_of_sub_path = std::search(base.begin(), base.end(), path.begin(), path.end());
        if (std::distance(begin_of_sub_path, base.end()) == std::distance(path.begin(), path.end()))
        {
            for (auto it = base.begin(); it != begin_of_sub_path; it++)
            {
                ret /= *it;
            }
        }
    }
    return ret;
}

std::string trim(std::string str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch)
                                        { return !std::isspace(ch); }));
    str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch)
                           { return !std::isspace(ch); })
                  .base(),
              str.end());
    return std::move(str);
}
std::string trim(std::string str, char to_strip)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [to_strip](unsigned char ch)
                                        { return ch != to_strip; }));
    str.erase(std::find_if(str.rbegin(), str.rend(), [to_strip](unsigned char ch)
                           { return ch != to_strip; })
                  .base(),
              str.end());
    return std::move(str);
}

std::string to_lower(std::string str)
{
    std::string lower_case{ std::move(str) };
    std::transform(lower_case.begin(), lower_case.end(), lower_case.begin(), [](char c)
                   { return static_cast<char>(std::tolower(c)); });
    return lower_case;
}
std::string to_upper(std::string str)
{
    std::string upper_case{ std::move(str) };
    std::transform(upper_case.begin(), upper_case.end(), upper_case.begin(), [](char c)
                   { return static_cast<char>(std::toupper(c)); });
    return upper_case;
}

bool case_insensitive_equal(std::string_view lhs, std::string_view rhs)
{
    struct case_insensitive_char_traits : public std::char_traits<char>
    {
        static bool eq(char c1, char c2)
        {
            return toupper(c1) == toupper(c2);
        }
        static bool ne(char c1, char c2)
        {
            return toupper(c1) != toupper(c2);
        }
        static bool lt(char c1, char c2)
        {
            return toupper(c1) < toupper(c2);
        }
        static int compare(const char* s1, const char* s2, size_t n)
        {
            while (n-- != 0)
            {
                if (toupper(*s1) < toupper(*s2))
                    return -1;
                if (toupper(*s1) > toupper(*s2))
                    return 1;
                ++s1;
                ++s2;
            }
            return 0;
        }
        static const char* find(const char* s, int n, char a)
        {
            while (n-- > 0 && toupper(*s) != toupper(a))
            {
                ++s;
            }
            return s;
        }
    };
    using case_insensitive_string_view = std::basic_string_view<char, case_insensitive_char_traits>;
    return case_insensitive_string_view{ lhs.data(), lhs.size() } == case_insensitive_string_view{ rhs.data(), rhs.size() };
}

template<typename T>
std::string to_utf8(const std::basic_string<T>& source)
{
    std::wstring_convert<std::codecvt_utf8_utf16<T>, T> convertor;
    return convertor.to_bytes(source);
}

template<typename T>
std::basic_string<T> from_utf8(const std::string& source)
{
    std::wstring_convert<std::codecvt_utf8_utf16<T>, T> convertor;
    return convertor.from_bytes(source);
}

template std::string to_utf8<char8_t>(const std::basic_string<char8_t>&);
template std::string to_utf8<char16_t>(const std::basic_string<char16_t>&);
template std::string to_utf8<char32_t>(const std::basic_string<char32_t>&);
template std::string to_utf8<wchar_t>(const std::basic_string<wchar_t>&);

template std::basic_string<char8_t> from_utf8(const std::string&);
template std::basic_string<char16_t> from_utf8(const std::string&);
template std::basic_string<char32_t> from_utf8(const std::string&);
template std::basic_string<wchar_t> from_utf8(const std::string&);
} // namespace algo
