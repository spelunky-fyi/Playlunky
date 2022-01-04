#include "dds_conversion.h"

#include "log.h"
#include "util/algorithms.h"
#include "util/color.h"
#include "util/image.h"
#include "util/on_scope_exit.h"
#include "util/span_util.h"

#include <cassert>
#include <fstream>
#include <span>

bool IsSupportedFileType(const std::filesystem::path& extension)
{
    using namespace std::string_view_literals;
    constexpr std::array supported_extensions = {
        ".bmp"sv,
        ".dib"sv,
        ".jpeg"sv,
        ".jpg"sv,
        ".jpe"sv,
        ".jp2"sv,
        ".png"sv,
        ".webp"sv,
        ".pbm"sv,
        ".pgm"sv,
        ".ppm"sv,
        ".sr"sv,
        ".ras"sv,
        ".tiff"sv,
        ".tif"sv,
    };
    const std::string ext_string = algo::to_lower(extension.string());
    return algo::contains(supported_extensions, ext_string);
}

bool ConvertRBGAToDds(std::span<const std::uint8_t> source, std::uint32_t width, std::uint32_t height, const std::filesystem::path& destination)
{
    namespace fs = std::filesystem;

    {
        const auto dest_parent_dir = destination.parent_path();
        if (!fs::exists(dest_parent_dir))
        {
            fs::create_directories(dest_parent_dir);
        }
    } // namespace std::filesystem;

    if (auto dest_file = std::ofstream{ destination, std::ios::trunc | std::ios::binary })
    {
        // https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dds-header
        const std::uint32_t header_size = 124;  // hardcoded
        const std::uint32_t flags = 0x0002100F; // required flags + pitch + mipmapped

        const std::uint32_t pitch = width * 4; // aka bytes per line
        const std::uint32_t depth = 1;
        const std::uint32_t mipmaps = 1;

        const std::uint32_t reserverd1[11]{};

        // pixel format sub structure
        const std::uint32_t pfsize = 32;    // size of pixel format structure, constant
        const std::uint32_t pfflags = 0x41; // uncompressed RGB with alpha channel
        const std::uint32_t fourcc = 0;     // compression mode (not used for uncompressed data)
        const std::uint32_t bitcount = 32;

        // bit masks for each channel, here for RGBA
        const std::uint32_t rmask = 0x000000FF;
        const std::uint32_t gmask = 0x0000FF00;
        const std::uint32_t bmask = 0x00FF0000;
        const std::uint32_t amask = 0xFF000000;

        const std::uint32_t caps = 0x1000; // simple texture with only one surface and no mipmaps
        const std::uint32_t caps2 = 0;     // additional surface data, unused
        const std::uint32_t caps3 = 0;     // unused
        const std::uint32_t caps4 = 0;     // unused

        const std::uint32_t reserved2 = 0;

        auto write_as_bytes = [](auto& stream, auto... datas)
        {
            (
                stream.write(reinterpret_cast<const char*>(&datas), sizeof(datas)),
                ...);
        };

        dest_file << "DDS "; // magic bytes
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

bool ConvertImageToDds(const std::filesystem::path& source, const std::filesystem::path& destination)
{
    Image source_image;
    if (source_image.Load(source))
    {
        return ConvertRBGAToDds(source_image.GetData(), source_image.GetWidth(), source_image.GetHeight(), destination);
    }
    return false;
}

bool ConvertDdsToPng(std::span<const std::uint8_t> source, const std::filesystem::path& destination)
{
    namespace fs = std::filesystem;

    if (!fs::exists(destination.parent_path()))
    {
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

    auto get_lowest_bit = [](std::uint32_t mask) -> std::uint8_t
    {
        for (std::uint8_t i = 0; i < 32; i++)
        {
            const std::uint32_t bit = 1 << (31 - i);
            const std::uint32_t masked = mask & bit;
            if (masked != 0)
            {
                return i;
            }
        }
        return 32;
    };
    const auto rshift = 32 - 8 - get_lowest_bit(rmask);
    const auto gshift = 32 - 8 - get_lowest_bit(gmask);
    const auto bshift = 32 - 8 - get_lowest_bit(bmask);
    const auto ashift = 32 - 8 - get_lowest_bit(amask);

    source = orig_source;             // back to beginning
    source = source.subspan(4 + 124); // magic bytes and whole header

    std::vector<std::uint8_t> image_buffer{ source.begin(), source.end() };
    auto image = span::bit_cast<ColorRGBA8>(image_buffer);
    for (auto& pixel : image)
    {
        std::uint32_t original_pixel = *reinterpret_cast<std::uint32_t*>(&pixel);
        pixel.r = static_cast<std::uint8_t>(original_pixel >> rshift);
        pixel.g = static_cast<std::uint8_t>(original_pixel >> gshift);
        pixel.b = static_cast<std::uint8_t>(original_pixel >> bshift);
        pixel.a = static_cast<std::uint8_t>(original_pixel >> ashift);
    }

    Image image_file;
    image_file.LoadRawData(image_buffer, width, height);
    return image_file.Write(destination);
}

bool ConvertDdsToPng(const std::filesystem::path& source, const std::filesystem::path& destination)
{
    FILE* file{ nullptr };
    auto error = fopen_s(&file, source.string().c_str(), "rb");
    if (error == 0 && file != nullptr)
    {
        auto close_file = OnScopeExit{ [file]()
                                       { fclose(file); } };

        fseek(file, 0, SEEK_END);
        const std::size_t file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        if (auto data = std::make_unique<std::uint8_t[]>(file_size))
        {
            const auto size_read = fread(data.get(), 1, file_size, file);
            return ConvertDdsToPng({ data.get(), file_size }, destination);
        }
    }
    return false;
}
