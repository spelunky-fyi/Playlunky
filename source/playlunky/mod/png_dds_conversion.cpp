#include "png_dds_conversion.h"

#include "log.h"

#include <lodepng.h>
#include <fstream>
#include <span>

bool ConvertPngToDds(const std::filesystem::path& source, const std::filesystem::path& destination)
{

	std::vector<std::uint8_t> image_buffer;
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t error = lodepng::decode(image_buffer, width, height, source.string(), LCT_RGBA, 8);
	if (error != 0) {
		return false;
	}

	struct ColorRGBA8 {
		std::uint8_t R;
		std::uint8_t G;
		std::uint8_t B;
		std::uint8_t A;
	};
	std::span<ColorRGBA8> image{ reinterpret_cast<ColorRGBA8*>(image_buffer.data()), image_buffer.size() / 4 };
	for (ColorRGBA8& pixel : image) {
		if (pixel.A == 0) {
			pixel = ColorRGBA8{};
		}
	}

    {
        const std::filesystem::path dest_parent_dir = destination.parent_path();
        if (!std::filesystem::exists(dest_parent_dir)) {
            std::filesystem::create_directories(dest_parent_dir);
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

        dest_file.write(reinterpret_cast<const char*>(image.data()), image.size() * sizeof(ColorRGBA8));

        dest_file.flush();
        return true;
	}

	return false;
}
