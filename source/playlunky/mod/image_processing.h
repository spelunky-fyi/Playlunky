#pragma once

#include "util/image.h"

#include <filesystem>
#include <vector>

Image GenerateStickerPixelArt(Image input_sprite, ImageSize target_size);
Image MakeCombinedMenuPetHeads(std::vector<std::pair<Image, std::filesystem::path>> pet_heads, ImageSize target_size);
Image ColorBlend(Image color_image, Image target_image);
