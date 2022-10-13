#include "level_parser.h"

#include "level_data.h"
#include "log.h"
#include "util/tokenize.h"
#include "virtual_filesystem.h"

#include <ranges>

LevelData LevelParser::LoadLevel(const VirtualFilesystem& vfs, const std::filesystem::path& backup_folder, const std::filesystem::path& level_file)
{
    return LoadLevel(vfs.GetFilePath(level_file).value_or(backup_folder / level_file));
}

LevelData LevelParser::LoadLevel(const std::filesystem::path& full_level_file)
{
    namespace fs = std::filesystem;
    using namespace std::string_view_literals;

    const std::string level_str{
        [&]
        {
            if (auto original_shader_file = std::ifstream{ full_level_file })
            {
                std::string contents((std::istreambuf_iterator<char>(original_shader_file)), std::istreambuf_iterator<char>());
                std::ranges::replace(contents, '\r', ' ');
                return contents;
            }
            return std::string{};
        }()
    };
    if (level_str.empty())
    {
        return LevelData{};
    }

    LevelData level_data{};
    level_data.Name = full_level_file.string();

    Tokenize<'\n', TokenizeBehavior::TrimWhitespace> lines{ level_str };
    auto lines_current{ lines.begin() };
    auto lines_end{ lines.end() };
    while (lines_current != lines_end)
    {
        const std::string_view line{ *lines_current };
        if (!line.empty())
        {
            const std::string_view code{ *Tokenize<"//", TokenizeBehavior::TrimWhitespace>{ line }.begin() };

            if (code.starts_with("\\"sv))
            {
                const char definition_prefix{ code[1] };
                const std::string_view definition{ code.substr(2) };
                switch (definition_prefix)
                {
                case '-':
                {
                    if (definition.starts_with("size"sv))
                    {
                        const std::string_view sizes{ algo::trim(definition.substr("size"sv.size())) };
                        const Tokenize<" \t", TokenizeBehavior::TrimWhitespace | TokenizeBehavior::SkipEmpty | TokenizeBehavior::AnyOfDelimiter>
                            sizes_split{ sizes };
                        const std::string_view width{ *sizes_split.begin() };
                        const std::string_view height{ *(++sizes_split.begin()) };

                        std::from_chars(width.data(), width.data() + width.size(), level_data.Width);
                        std::from_chars(height.data(), height.data() + height.size(), level_data.Height);
                    }
                    else
                    {
                        const Tokenize<" \t", TokenizeBehavior::TrimWhitespace | TokenizeBehavior::SkipEmpty | TokenizeBehavior::AnyOfDelimiter>
                            setting_split{ definition };
                        const std::string_view name{ *setting_split.begin() };
                        const std::string_view value{ *(++setting_split.begin()) };

                        LevelSetting setting{ .Name{ std::string{ name } } };
                        std::from_chars(value.data(), value.data() + value.size(), setting.Value);
                        level_data.Settings.push_back(std::move(setting));
                    }
                    break;
                }
                case '?':
                {
                    const Tokenize<" \t", TokenizeBehavior::TrimWhitespace | TokenizeBehavior::SkipEmpty | TokenizeBehavior::AnyOfDelimiter>
                        tilecode_split{ definition };
                    const std::string_view full_code{ *tilecode_split.begin() };
                    const std::vector<std::string_view> full_code_split{ algo::split<'%'>(full_code) };
                    const std::string_view short_code{ *(++tilecode_split.begin()) };
                    TileCode tile_code{
                        .ShortCode{ static_cast<std::uint8_t>(short_code[0]) },
                        .TileOne{ std::string{ full_code_split[0] } },
                    };
                    if (full_code_split.size() > 1)
                    {
                        std::from_chars(full_code_split[1].data(), full_code_split[1].data() + full_code_split[1].size(), tile_code.Chance);
                    }
                    if (full_code_split.size() > 2)
                    {
                        tile_code.TileTwo = std::string{ full_code_split[2] };
                    }
                    level_data.TileCodes.push_back(std::move(tile_code));
                    break;
                }
                case '%':
                    [[fallthrough]];
                case '+':
                {
                    const Tokenize<" \t", TokenizeBehavior::TrimWhitespace | TokenizeBehavior::SkipEmpty | TokenizeBehavior::AnyOfDelimiter, 2> chance_split{ definition };
                    const std::string_view name{ *chance_split.begin() };
                    const std::string_view chances{ *(++chance_split.begin()) };

                    LevelChance level_chance{
                        .Name{ std::string{ name } },
                    };
                    for (std::string_view chance : Tokenize<',', TokenizeBehavior::TrimWhitespace>{ chances })
                    {
                        level_chance.Chances.push_back(0);
                        std::from_chars(chance.data(), chance.data() + chance.size(), level_chance.Chances.back());
                    }

                    auto& level_chances{
                        definition_prefix == '%'
                            ? level_data.Chances
                            : level_data.MonsterChances
                    };
                    level_chances.push_back(std::move(level_chance));
                    break;
                }
                case '.':
                {
                    LevelRoom room_data{};
                    room_data.Name = definition;
                    lines_current++;
                    while (lines_current != lines_end)
                    {
                        const std::string_view room_line{ *lines_current };
                        if (!room_data.FrontData.empty())
                        {
                            if (room_line.empty() || room_line.starts_with("\\!"))
                            {
                                level_data.Rooms.push_back(std::move(room_data));
                                room_data = LevelRoom{};
                                room_data.Name = definition;
                                continue;
                            }
                        }

                        if (room_line.starts_with("\\."))
                        {
                            // Let this line be handled by the main loop instead, i.e. jump out without incrementing the iterator
                            break;
                        }

                        if (!room_line.empty())
                        {
                            const std::string_view room_code{ *Tokenize<"//", TokenizeBehavior::TrimWhitespace>{ room_line }.begin() };

                            if (!room_code.empty())
                            {
                                if (room_code.starts_with("\\"sv))
                                {
                                    const char room_line_prefix{ room_code[1] };
                                    const std::string_view room_line_definition{ room_code.substr(2) };

                                    switch (room_line_prefix)
                                    {
                                    case '!':
                                    {
                                        room_data.Flags.emplace_back(room_line_definition);
                                        break;
                                    }
                                    default:
                                    {
                                        LogError("Unexpected line in level file: \"{}\"", room_line);
                                    }
                                    }
                                }
                                else
                                {
                                    const Tokenize<" \t", TokenizeBehavior::TrimWhitespace | TokenizeBehavior::SkipEmpty | TokenizeBehavior::AnyOfDelimiter>
                                        room_line_split{ room_line };
                                    auto room_line_it{ room_line_split.begin() };

                                    const std::string_view front_layer{ *room_line_it };
                                    for (char c : front_layer)
                                    {
                                        room_data.FrontData.push_back(static_cast<std::uint8_t>(c));
                                    }

                                    room_line_it++;
                                    if (room_line_it != room_line_split.end())
                                    {
                                        const std::string_view back_layer{ *room_line_it };
                                        for (char c : back_layer)
                                        {
                                            room_data.BackData.push_back(static_cast<std::uint8_t>(c));
                                        }
                                    }

                                    room_data.Width = static_cast<uint32_t>(front_layer.size());
                                    room_data.Height++;
                                }
                            }
                        }
                        lines_current++;
                    }

                    // Reached EoF
                    if (lines_current == lines_end && !room_data.Name.empty())
                    {
                        level_data.Rooms.push_back(std::move(room_data));
                    }

                    continue;
                }
                default:
                {
                    LogError("Unexpected line in level file: \"{}\"", line);
                }
                }
            }
        }

        lines_current++;
    }

    return level_data;
}
