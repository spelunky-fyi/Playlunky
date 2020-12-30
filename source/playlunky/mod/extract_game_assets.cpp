#include "extract_game_assets.h"

#include "log.h"
#include "chacha.h"
#include "png_dds_conversion.h"
#include "detour/sigscan.h"
#include "util/algorithms.h"

#include <cassert>
#include <fstream>
#include <span>
#include <zstd.h>

template<class T>
T Read(void*& data) {
    T value = *(T*)data;
    data = (void*)((char*)data + sizeof(T));
    return value;
}
template<class T>
std::vector<T> Read(void*& data, std::size_t size) {
    std::vector<T> value(size);
    std::memcpy(value.data(), data, size * sizeof(T));
    data = (void*)((char*)data + size * sizeof(T));
    return value;
}

bool ExtractGameAssets(std::span<const std::filesystem::path> files, const std::filesystem::path& destination) {
    namespace fs = std::filesystem;

	if (files.empty()) {
		return true;
	}

    std::vector<std::filesystem::path> full_file_paths(files.size());
    for (size_t i = 0; i < files.size(); i++) {
        full_file_paths[i] = destination / files[i];
        if (fs::exists(full_file_paths[i])) {
            full_file_paths[i].clear();
        }
    }

    if (algo::contains_if(full_file_paths, [](auto& path) { return path.empty(); })) {
        return true;
    }

	if (void* data = SigScan::GetDataSection()) {
        struct Asset {
            void* Data;
            std::size_t DataSize;
            ChaCha::bytes_t AssetNameHash;
            bool Encrypted;
        };
        std::vector<Asset> assets;

        ChaCha::Key key;
        while (true) {
            const auto asset_len = Read<std::uint32_t>(data);
            const auto asset_name_len = Read<std::uint32_t>(data);
            if (asset_len == 0 && asset_name_len == 0) {
                break;
            }

            const auto asset_name_hash = Read<std::uint8_t>(data, asset_name_len);
            const auto encrypted = Read<char>(data) == '\x01';

            const auto data_address = data;
            const auto data_size = asset_len - 1;

            (void)Read<char>(data, data_size);

            key.update(asset_len);

            assets.push_back(Asset{
                .Data = data_address,
                .DataSize = data_size,
                .AssetNameHash = asset_name_hash,
                .Encrypted = encrypted
            });
        }

        const ChaCha::bytes_t empty_hash{ unsigned char('\xDE'), unsigned char('\xAD'), unsigned char('\xBE'), unsigned char('\xEF') };

        std::vector<ChaCha::bytes_t> hashes(files.size());
        for (std::size_t i = 0; i < files.size(); i++) {
            if (full_file_paths[i].empty()) {
                hashes[i] = empty_hash;
            }
            else {
                const auto file_string = files[i].string();
                hashes[i] = ChaCha::hash_filepath(file_string, key.Current);
            }
        }

        for (const Asset& asset : assets) {
            for (std::size_t i = 0; i < hashes.size(); i++) {
                if (hashes[i] == asset.AssetNameHash) {
                    const auto& file = files[i];
                    const auto& file_path = file.string();
                    ChaCha::bytes_t decryped_data;
                    std::span<const std::uint8_t> asset_data{ (std::uint8_t*)asset.Data, asset.DataSize };
                    if (asset.Encrypted) {
                        decryped_data = ChaCha::chacha(file_path, asset_data, key.Current);

                        const std::uint64_t decompressed_size = ZSTD_getFrameContentSize(decryped_data.data(), decryped_data.size());
                        assert(decompressed_size != ZSTD_CONTENTSIZE_ERROR);
                        assert(decompressed_size != ZSTD_CONTENTSIZE_UNKNOWN);

                        ChaCha::bytes_t decompressed_data(decompressed_size);
                        const std::size_t decompressed_read_size = ZSTD_decompress(decompressed_data.data(), decompressed_data.size(), decryped_data.data(), decryped_data.size());

                        decryped_data = std::move(decompressed_data);
                        asset_data = decryped_data;
                    }

                    const auto& full_destination = full_file_paths[i];
                    
                    {
                        const auto full_destination_dir = full_destination.parent_path();
                        if (!fs::exists(full_destination.parent_path())) {
                            fs::create_directories(full_destination.parent_path());
                        }
                    }

                    if (auto out_file = std::ofstream{ full_destination, std::ios::binary | std::ios::trunc }) {
                        out_file.write(reinterpret_cast<const char*>(asset_data.data()), asset_data.size());
                    }

                    if (file.extension() == ".DDS") {
                        auto converted_file = full_destination;
                        converted_file.replace_extension(".png");
                        ConvertDdsToPng(asset_data, converted_file);
                    }

                    hashes[i] = empty_hash;
                    break;
                }
            }
        }
	}

    return true;
}
