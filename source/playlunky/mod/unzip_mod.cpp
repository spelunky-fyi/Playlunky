#include "unzip_mod.h"

#include "log.h"
#include "util/unzip_file.h"
#include "util/format.h"

void UnzipMod(const std::filesystem::path& zip_file) {
	namespace fs = std::filesystem;

	const auto mod_folder = fs::path{ zip_file }.replace_extension("");
	if (ZipError err = UnzipFile(zip_file, mod_folder)) {
		LogError("Can't open zip archive '{}': {}/n", zip_file.string(), err.value());
	}
}