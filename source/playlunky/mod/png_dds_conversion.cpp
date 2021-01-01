#include "png_dds_conversion.h"

#include "log.h"
#include "util/on_scope_exit.h"

#include <lodepng.h>
#include <cassert>
#include <fstream>
#include <span>

struct ColorRGBA8 {
    std::uint8_t R;
    std::uint8_t G;
    std::uint8_t B;
    std::uint8_t A;
};

template<class T>
auto ByteCast(std::span<std::uint8_t> data) {
    return std::span<T>{ reinterpret_cast<T*>(data.data()), data.size() / sizeof(T) };
}

bool ConvertRBGAToDds(std::span<const std::uint8_t> source, std::uint32_t width, std::uint32_t height, const std::filesystem::path& destination) {
    namespace fs = std::filesystem;

    {
        const auto dest_parent_dir = destination.parent_path();
        if (!fs::exists(dest_parent_dir)) {
            fs::create_directories(dest_parent_dir);
        }
    }

    if (auto dest_file = std::ofstream{ destination, std::ios::trunc | std::ios::binary }) {
        // https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dds-header
        const std::uint32_t header_size = 124;  // hardcoded
        const std::uint32_t flags = 0x0002100F;  // required flags + pitch + mipmapped

        const std::uint32_t pitch = width * 4;  // aka bytes per line
        const std::uint32_t depth = 1;
        const std::uint32_t mipmaps = 1;

        const std::uint32_t reserverd1[11]{};

        // pixel format sub structure
        const std::uint32_t pfsize = 32;  // size of pixel format structure, constant
        const std::uint32_t pfflags = 0x41;  // uncompressed RGB with alpha channel
        const std::uint32_t fourcc = 0;  // compression mode (not used for uncompressed data)
        const std::uint32_t bitcount = 32;

        // bit masks for each channel, here for RGBA
        const std::uint32_t rmask = 0x000000FF;
        const std::uint32_t gmask = 0x0000FF00;
        const std::uint32_t bmask = 0x00FF0000;
        const std::uint32_t amask = 0xFF000000;

        const std::uint32_t caps = 0x1000;  // simple texture with only one surface and no mipmaps
        const std::uint32_t caps2 = 0;  // additional surface data, unused
        const std::uint32_t caps3 = 0;  // unused
        const std::uint32_t caps4 = 0;  // unused

        const std::uint32_t reserved2 = 0;

        auto write_as_bytes = [](auto& stream, auto... datas) {
            (
                stream.write(reinterpret_cast<const char*>(&datas), sizeof(datas)),
                ...
                );
        };

        dest_file << "DDS ";  // magic bytes
        write_as_bytes(dest_file, header_size, flags);
        write_as_bytes(dest_file, height, width, pitch, depth, mipmaps);
        dest_file.write(reinterpret_cast<const char*>(reserverd1), sizeof(reserverd1));
        write_as_bytes(dest_file, pfsize, pfflags, fourcc, bitcount);
        write_as_bytes(dest_file, rmask, gmask, bmask, amask);
        write_as_bytes(dest_file, caps, caps2, caps3, caps4);
        write_as_bytes(dest_file, reserved2);

        dest_file.write(reinterpret_cast<const char*>(source.data()), source.size());

        dest_file.flush();
        return true;
    }

    return false;
}

bool ConvertPngToDds(const std::filesystem::path& source, const std::filesystem::path& destination)
{
	std::vector<std::uint8_t> image_buffer;
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t error = lodepng::decode(image_buffer, width, height, source.string(), LCT_RGBA, 8);
	if (error != 0) {
		return false;
	}

	auto image = ByteCast<ColorRGBA8>(image_buffer);
	for (ColorRGBA8& pixel : image) {
		if (pixel.A == 0) {
			pixel = ColorRGBA8{};
		}
	}

    return ConvertRBGAToDds(image_buffer, width, height, destination);
}

bool ConvertDdsToPng(std::span<const std::uint8_t> source, const std::filesystem::path& destination) {
    namespace fs = std::filesystem;

    if (!fs::exists(destination.parent_path())) {
        fs::create_directories(destination.parent_path());
    }

    const auto orig_source = source;

    {
        const auto magic_bytes = std::string_view{ reinterpret_cast<const char*>(source.data()), 4 };
        assert(magic_bytes == "DDS ");
        source = source.subspan(4);

        assert(*reinterpret_cast<const std::uint32_t*>(source.data()) == 124);
        source = source.subspan(4); // header_size
        source = source.subspan(4); // flags
    }

    const auto height = *reinterpret_cast<const std::uint32_t*>(source.data());
    source = source.subspan(4); // height
    const auto width = *reinterpret_cast<const std::uint32_t*>(source.data());
    source = source.subspan(4 * (8 + 11)); // width until bitcount

    const auto rmask = *reinterpret_cast<const std::uint32_t*>(source.data());
    source = source.subspan(4); // rmask
    const auto gmask = *reinterpret_cast<const std::uint32_t*>(source.data());
    source = source.subspan(4); // gmask
    const auto bmask = *reinterpret_cast<const std::uint32_t*>(source.data());
    source = source.subspan(4); // bmask
    const auto amask = *reinterpret_cast<const std::uint32_t*>(source.data());

    auto get_lowest_bit = [](std::uint32_t mask) -> std::uint8_t {
        for (std::uint8_t i = 0; i < 32; i++) {
            const std::uint32_t bit = 1 << (31 - i);
            const std::uint32_t masked = mask & bit;
            if (masked != 0) {
                return i;
            }
        }
        return 32;
    };
    const auto rshift = 32 - 8 - get_lowest_bit(rmask);
    const auto gshift = 32 - 8 - get_lowest_bit(gmask);
    const auto bshift = 32 - 8 - get_lowest_bit(bmask);
    const auto ashift = 32 - 8 - get_lowest_bit(amask);

    source = orig_source; // back to beginning
    source = source.subspan(4 + 124); // magic bytes and whole header

    std::vector<std::uint8_t> image_buffer{ source.begin(), source.end() };
    auto image = ByteCast<ColorRGBA8>(image_buffer);
    for (auto& pixel : image) {
        std::uint32_t original_pixel = *reinterpret_cast<std::uint32_t*>(&pixel);
        pixel.R = static_cast<std::uint8_t>(original_pixel >> rshift);
        pixel.G = static_cast<std::uint8_t>(original_pixel >> gshift);
        pixel.B = static_cast<std::uint8_t>(original_pixel >> bshift);
        pixel.A = static_cast<std::uint8_t>(original_pixel >> ashift);
    }

    std::uint32_t error = lodepng::encode(destination.string(), image_buffer, width, height);
    if (error != 0) {
        return false;
    }

    return true;
}

bool ConvertDdsToPng(const std::filesystem::path& source, const std::filesystem::path& destination) {
    FILE* file{ nullptr };
    auto error = fopen_s(&file, source.string().c_str(), "rb");
    if (error == 0 && file != nullptr) {
        auto close_file = OnScopeExit{ [file]() { fclose(file); } };

        fseek(file, 0, SEEK_END);
        const std::size_t file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        if (auto data = std::make_unique<std::uint8_t[]>(file_size)) {
            const auto size_read = fread(data.get(), 1, file_size, file);
            return ConvertDdsToPng({ data.get(), file_size }, destination);
        }
    }
    return false;
}
