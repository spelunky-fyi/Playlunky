#pragma once

#include "util/color.h"
#include "util/image.h"

#include <filesystem>
#include <vector>

Image GenerateStickerPixelArt(Image input_sprite, ImageSize target_size);
Image MakeCombinedMenuPetHeads(std::vector<std::pair<Image, std::filesystem::path>> pet_heads, ImageSize target_size);

// Blending according to Aseprite blend modes
Image ColorBlend(Image color_image, Image target_image);
Image LuminanceBlend(Image luminance_image, Image target_image);

std::vector<ColorRGB8> GetUniqueColors(const Image& image, std::size_t max_numbers = 16);
