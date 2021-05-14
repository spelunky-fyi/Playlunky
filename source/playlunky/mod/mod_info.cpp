#include "mod_info.h"

#include <fstream>

#include <nlohmann/json.hpp>

void from_json(const nlohmann::json& j, Tile& tile)
{
    if (j.is_array())
    {
        tile.Left = j[0];
        tile.Top = j[1];
        tile.Right = j[2];
        tile.Bottom = j[3];
    }
    else if (j.is_object())
    {
        tile.Left = j["left"];
        tile.Right = j["right"];
        tile.Top = j["top"];
        tile.Bottom = j["bottom"];
    }
    else
    {
        tile = {};
    }
}
void from_json(const nlohmann::json& j, TileMapping& tile_mapping)
{
    tile_mapping.SourceTile = j["from"];
    tile_mapping.TargetTile = j["to"];
}

ModInfo::ModInfo(std::string name)
    : mName{ std::move(name) }
{
}

void ModInfo::ReadExtendedInfoFromJson(std::string_view path)
{
    if (auto json_file = std::ifstream(path))
    {
        mExtendedVersionAvailable = true;

        auto json = nlohmann::json::parse(json_file);
        auto get_if_contained = [&json]<class T>(T& value, auto name)
        {
            if (json.contains(name))
            {
                json[name].get_to(value);
            }
        };

        get_if_contained(mName, "name");
        get_if_contained(mDescription, "description");
        get_if_contained(mAuthor, "author");
        get_if_contained(mVersion, "version");

        get_if_contained(mCustomImages, "image_map");
    }
}
