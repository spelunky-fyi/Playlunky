#pragma once

#include <filesystem>
#include <optional>
#include <string>

using ZipError = std::optional<std::string>;
ZipError UnzipFile(const std::filesystem::path& source_file, const std::filesystem::path& destination_folder);
