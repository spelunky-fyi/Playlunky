#pragma once

#include "util/color.h"
#include "util/image.h"

#include <filesystem>
#include <vector>

Image GenerateStickerPixelArt(Image input_sprite, ImageSize target_size);
Image MakeCombinedMenuPetHeads(std::vector<std::pair<Image, std::filesystem::path>> pet_heads, ImageSize target_size);

// Blending according to Aseprite blend modes
Image AlphaBlend(Image lhs_image, Image rhs_image);
Image ColorBlend(Image color_image, Image target_image);
Image LuminanceBlend(Image luminance_image, Image target_image);
Image LuminanceScale(Image luminance_scale_image, Image target_image);

Image ReplaceColor(Image input_image, ColorRGB8 source_color, ColorRGB8 target_color);
Image ReplaceColors(Image input_image, const std::vector<ColorRGB8>& source_colors, const std::vector<ColorRGB8>& target_colors);
Image ExtractColor(Image input_image, ColorRGB8 color);
