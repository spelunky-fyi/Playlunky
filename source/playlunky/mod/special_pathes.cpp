#include "special_pathes.h"

#include "mod/virtual_filesystem.h"
#include "playlunky_settings.h"
#include "util/image.h"

void SetupSpecialPathes(VirtualFilesystem& vfs, const PlaylunkySettings& settings)
{
    {
        // Bind char pathes
        vfs.BindPathes({ "Data/Textures/char_orange", "Data/Textures/Entities/char_orange_full" });
        vfs.BindPathes({ "Data/Textures/char_pink", "Data/Textures/Entities/char_pink_full" });
        vfs.BindPathes({ "Data/Textures/char_red", "Data/Textures/Entities/char_red_full" });
        vfs.BindPathes({ "Data/Textures/char_violet", "Data/Textures/Entities/char_violet_full" });
        vfs.BindPathes({ "Data/Textures/char_white", "Data/Textures/Entities/char_white_full" });
        vfs.BindPathes({ "Data/Textures/char_yellow", "Data/Textures/Entities/char_yellow_full" });
        vfs.BindPathes({ "Data/Textures/char_black", "Data/Textures/Entities/char_black_full" });
        vfs.BindPathes({ "Data/Textures/char_blue", "Data/Textures/Entities/char_blue_full" });
        vfs.BindPathes({ "Data/Textures/char_cerulean", "Data/Textures/Entities/char_cerulean_full" });
        vfs.BindPathes({ "Data/Textures/char_cinnabar", "Data/Textures/Entities/char_cinnabar_full" });
        vfs.BindPathes({ "Data/Textures/char_cyan", "Data/Textures/Entities/char_cyan_full" });
        vfs.BindPathes({ "Data/Textures/char_eggchild", "Data/Textures/Entities/char_eggchild_full" });
        vfs.BindPathes({ "Data/Textures/char_gold", "Data/Textures/Entities/char_gold_full" });
        vfs.BindPathes({ "Data/Textures/char_gray", "Data/Textures/Entities/char_gray_full" });
        vfs.BindPathes({ "Data/Textures/char_green", "Data/Textures/Entities/char_green_full" });
        vfs.BindPathes({ "Data/Textures/char_hired", "Data/Textures/Entities/char_hired_full" });
        vfs.BindPathes({ "Data/Textures/char_iris", "Data/Textures/Entities/char_iris_full" });
        vfs.BindPathes({ "Data/Textures/char_khaki", "Data/Textures/Entities/char_khaki_full" });
        vfs.BindPathes({ "Data/Textures/char_lemon", "Data/Textures/Entities/char_lemon_full" });
        vfs.BindPathes({ "Data/Textures/char_lime", "Data/Textures/Entities/char_lime_full" });
        vfs.BindPathes({ "Data/Textures/char_magenta", "Data/Textures/Entities/char_magenta_full" });
        vfs.BindPathes({ "Data/Textures/char_olive", "Data/Textures/Entities/char_olive_full" });

        vfs.BindPathes({ "Data/Textures/Entities/monty_full", "Data/Textures/Entities/Pets/monty", "Data/Textures/Entities/Pets/monty_v2" });
        vfs.BindPathes({ "Data/Textures/Entities/percy_full", "Data/Textures/Entities/Pets/percy", "Data/Textures/Entities/Pets/percy_v2" });
        vfs.BindPathes({ "Data/Textures/Entities/poochi_full", "Data/Textures/Entities/Pets/poochi", "Data/Textures/Entities/Pets/poochi_v2" });
        vfs.BindPathes({ "Data/Textures/Entities/turkey_full", "Data/Textures/Entities/Mounts/turkey" });
        vfs.BindPathes({ "Data/Textures/Entities/rockdog_full", "Data/Textures/Entities/Mounts/rockdog" });
        vfs.BindPathes({ "Data/Textures/Entities/axolotl_full", "Data/Textures/Entities/Mounts/axolotl" });
        vfs.BindPathes({ "Data/Textures/Entities/qilin_full", "Data/Textures/Entities/Mounts/qilin" });
    }

    if (const bool link_related_files = settings.GetBool("sprite_settings", "link_related_files", true))
    {
        // Link char paths and char name/color/json
        vfs.LinkPathes({
            { "Data/Textures/char_orange", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_orange_full", Image::AllowedExtensions },
            { "char_orange" },
        });
        vfs.LinkPathes({
            { "Data/Textures/char_pink", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_pink_full", Image::AllowedExtensions },
            { "char_pink" },
        });
        vfs.LinkPathes({
            { "Data/Textures/char_red", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_red_full", Image::AllowedExtensions },
            { "char_red" },
        });
        vfs.LinkPathes({
            { "Data/Textures/char_violet", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_violet_full", Image::AllowedExtensions },
            { "char_violet" },
        });
        vfs.LinkPathes({
            { "Data/Textures/char_white", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_white_full", Image::AllowedExtensions },
            { "char_white" },
        });
        vfs.LinkPathes({
            { "Data/Textures/char_yellow", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_yellow_full", Image::AllowedExtensions },
            { "char_yellow" },
        });
        vfs.LinkPathes({
            { "Data/Textures/char_black", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_black_full", Image::AllowedExtensions },
            { "char_black" },
        });
        vfs.LinkPathes({
            { "Data/Textures/char_blue", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_blue_full", Image::AllowedExtensions },
            { "char_blue" },
        });
        vfs.LinkPathes({
            { "Data/Textures/char_cerulean", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_cerulean_full", Image::AllowedExtensions },
            { "char_cerulean" },
        });
        vfs.LinkPathes({
            { "Data/Textures/char_cinnabar", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_cinnabar_full", Image::AllowedExtensions },
            { "char_cinnabar" },
        });
        vfs.LinkPathes({
            { "Data/Textures/char_cyan", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_cyan_full", Image::AllowedExtensions },
            { "char_cyan" },
        });
        vfs.LinkPathes({
            { "Data/Textures/char_gold", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_gold_full", Image::AllowedExtensions },
            { "char_gold" },
        });
        vfs.LinkPathes({
            { "Data/Textures/char_gray", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_gray_full", Image::AllowedExtensions },
            { "char_gray" },
        });
        vfs.LinkPathes({
            { "Data/Textures/char_green", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_green_full", Image::AllowedExtensions },
            { "char_green" },
        });
        vfs.LinkPathes({
            { "Data/Textures/char_iris", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_iris_full", Image::AllowedExtensions },
            { "char_iris" },
        });
        vfs.LinkPathes({
            { "Data/Textures/char_khaki", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_khaki_full", Image::AllowedExtensions },
            { "char_khaki" },
        });
        vfs.LinkPathes({
            { "Data/Textures/char_lemon", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_lemon_full", Image::AllowedExtensions },
            { "char_lemon" },
        });
        vfs.LinkPathes({
            { "Data/Textures/char_lime", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_lime_full", Image::AllowedExtensions },
            { "char_lime" },
        });
        vfs.LinkPathes({
            { "Data/Textures/char_magenta", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_magenta_full", Image::AllowedExtensions },
            { "char_magenta" },
        });
        vfs.LinkPathes({
            { "Data/Textures/char_olive", Image::AllowedExtensions },
            { "Data/Textures/Entities/char_olive_full", Image::AllowedExtensions },
            { "char_olive" },
        });
    }
}