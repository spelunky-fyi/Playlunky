#pragma once

#include <stdexcept>
#include <string_view>
#include <vector>

#include "algorithms.h"

template<size_t N>
struct TokenizeDelimiter
{
    constexpr TokenizeDelimiter(const char (&str)[N])
    {
        for (size_t i = 0; i < N; i++)
        {
            Str[i] = str[i];
        }
    };
    constexpr TokenizeDelimiter(char c)
    {
        Str[0] = c;
        Str[1] = '\0';
    };

    char Str[N]{};
    size_t Size{ N - 1 };
};

template<size_t N = 2>
TokenizeDelimiter(char) -> TokenizeDelimiter<2>;

enum class TokenizeBehavior
{
    None = 0,
    TrimWhitespace = 1 << 0,
    SkipEmpty = 1 << 1,
    AnyOfDelimiter = 1 << 2,
};
constexpr TokenizeBehavior operator|(TokenizeBehavior rhs, TokenizeBehavior lhs)
{
    return (TokenizeBehavior)((int)lhs | (int)rhs);
}
constexpr bool operator&(TokenizeBehavior rhs, TokenizeBehavior lhs)
{
    return ((int)lhs & (int)rhs) != 0;
}

template<TokenizeDelimiter Delimiter, TokenizeBehavior Behavior = TokenizeBehavior::None, size_t MaxTokens = std::string_view::npos>
class Tokenize
{
  public:
    Tokenize() = delete;
    constexpr Tokenize(const Tokenize&) = default;
    constexpr Tokenize(Tokenize&&) = default;
    constexpr Tokenize& operator=(const Tokenize&) = default;
    constexpr Tokenize& operator=(Tokenize&&) = default;

    explicit constexpr Tokenize(std::nullptr_t)
        : m_Source{}
    {
    }
    explicit constexpr Tokenize(const char* source)
        : m_Source{ source }
    {
        GetNext();
        m_NumTokens++;
    }
    explicit constexpr Tokenize(std::string_view source)
        : m_Source{ source }
    {
        GetNext();
        m_NumTokens++;
    }

    constexpr bool operator==(const Tokenize& rhs) const = default;
    constexpr bool operator!=(const Tokenize& rhs) const = default;

    constexpr auto begin()
    {
        return *this;
    }
    constexpr auto end()
    {
        return Tokenize{ m_Source, end_tag_t{} };
    }
    constexpr auto begin() const
    {
        return *this;
    }
    constexpr auto end() const
    {
        return Tokenize{ m_Source, end_tag_t{} };
    }
    constexpr auto cbegin() const
    {
        return *this;
    }
    constexpr auto cend() const
    {
        return Tokenize{ m_Source, end_tag_t{} };
    }

    constexpr auto operator*() const
    {
        const size_t pos{
            m_Position == 0 || (Behavior & TokenizeBehavior::AnyOfDelimiter)
                ? m_Position
                : m_Position + Delimiter.Size - 1
        };
        const std::string_view res{ m_Source.substr(pos, m_Next - pos) };
        if constexpr (Behavior & TokenizeBehavior::TrimWhitespace)
        {
            return algo::trim(res);
        }
        else
        {
            return res;
        }
    }

    constexpr decltype(auto) operator++()
    {
        if (!Advance())
            *this = end();
        return *this;
    }
    constexpr auto operator++(int)
    {
        auto copy = *this;
        ++(*this);
        return copy;
    }

  private:
    struct end_tag_t
    {
    };
    constexpr Tokenize(std::string_view source, end_tag_t)
        : m_Source{ source }
        , m_Position{ source.size() }
        , m_Next{ source.size() }
    {
    }

    constexpr void GetNext()
    {
        if constexpr (MaxTokens != std::string_view::npos)
        {
            if (m_NumTokens == MaxTokens)
            {
                m_Next = m_Source.size();
                return;
            }
        }

        m_Next = m_Position;
        if constexpr (Behavior & TokenizeBehavior::AnyOfDelimiter)
        {
            while (m_Next < m_Source.size() && !algo::contains(Delimiter.Str, m_Source[m_Next]))
            {
                ++m_Next;
            }
        }
        else
        {
            while (m_Next < m_Source.size() && !m_Source.substr(m_Next).starts_with(Delimiter.Str))
            {
                ++m_Next;
            }
        }
    }
    constexpr bool Advance()
    {
        m_Position = m_Next + 1;
        GetNext();

        if constexpr (Behavior & TokenizeBehavior::SkipEmpty)
        {
            while (m_Next == m_Position && m_Position < m_Source.size())
            {
                m_Position = m_Next + 1;
                GetNext();
            }
        }

        m_NumTokens++;
        return m_Position < m_Source.size();
    }

    std::string_view m_Source;
    size_t m_Position{ 0 };
    size_t m_Next{ 0 };
    size_t m_NumTokens{ 1 };
};

namespace algo
{
template<char Delimeter>
std::vector<std::string_view> split(std::string_view str)
{
    std::vector<std::string_view> sub_strings;
    for (auto&& sub_string : Tokenize<Delimeter>{ str })
    {
        sub_strings.push_back(sub_string);
    }
    return sub_strings;
}
} // namespace algo
