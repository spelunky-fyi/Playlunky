#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

std::uint32_t HashString(std::string_view string);

bool CreateHashedStringsFile(const std::filesystem::path& source_file, const std::filesystem::path& destination_file);
