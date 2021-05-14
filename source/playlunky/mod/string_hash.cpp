#include "string_hash.h"

#include "util/algorithms.h"
#include "util/regex.h"

#include <array>
#include <fstream>
#include <span>
#include <zlib.h>

static constexpr ctll::fixed_string s_CommentBlockRule{ "^#+" };

std::uint32_t HashString(std::string_view string)
{
    return crc32(0, reinterpret_cast<const Bytef*>(string.data()), static_cast<std::uint32_t>(string.size()));
}

bool CreateHashedStringsFile(const std::filesystem::path& source_file, const std::filesystem::path& destination_file)
{
    namespace fs = std::filesystem;
    {
        const auto destination_parent_folder = destination_file.parent_path();
        if (!fs::exists(destination_parent_folder))
        {
            fs::create_directories(destination_parent_folder);
        }
    } // namespace std::filesystem;

    if (auto source = std::ifstream{ source_file })
    {
        if (auto destination = std::ofstream{ destination_file, std::ios::trunc })
        {
            destination << std::hex << std::setfill('0');

            std::string current_comment_block;
            while (!source.eof())
            {
                std::string line;
                std::getline(source, line);
                if (ctre::starts_with<s_CommentBlockRule>(line))
                {
                    std::string comment_block = algo::trim(algo::trim(line, '#'));
                    if (!comment_block.empty())
                    {
                        current_comment_block = std::move(comment_block);
                    }
                    destination << "0x" << std::setw(8) << 0xdeadbeef << '\n';
                }
                else
                {
                    const auto comment_and_line = algo::trim(line) + current_comment_block;
                    destination << "0x" << std::setw(8) << HashString(comment_and_line) << '\n';
                }
            }
        }
    }

    return true;
}
