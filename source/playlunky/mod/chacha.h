#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace ChaCha
{
// This implementation is a direct translation of Modlunky's chacha source
// For reference see: https://github.com/spelunky-fyi/modlunky2/blob/1dd9acf7ae779d7ead82e7d74e2319dd46fef909/src/modlunky2/assets/chacha.py

using bytes_t = std::vector<uint8_t>;

enum class Version
{
    V1,
    V2
};
bytes_t hash_filepath(std::string_view filepath, std::uint64_t key, Version version = Version::V2);
bytes_t chacha(std::string_view filepath, std::span<const std::uint8_t> data, std::uint64_t key, Version version = Version::V2);

struct Key
{
    std::uint64_t Current{ 0 };

    void update(std::uint64_t asset_len);
};
} // namespace ChaCha