#include "mod_info.h"

#include "mod_database.h"

#include <fstream>

#include <nlohmann/json.hpp>
#include <zip_adaptor.h>

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
void from_json(const nlohmann::json& j, CustomImage& custom_image)
{
    j.get_to(custom_image.ImageMap);
    custom_image.Outdated = false;
}

void to_json(nlohmann::json& j, const Tile& tile)
{
    j = nlohmann::json::array({ tile.Left, tile.Top, tile.Right, tile.Bottom });
}
void to_json(nlohmann::json& j, const TileMapping& tile_mapping)
{
    j["from"] = tile_mapping.SourceTile;
    j["to"] = tile_mapping.TargetTile;
}
void to_json(nlohmann::json& j, const CustomImage& custom_image)
{
    j = custom_image.ImageMap;
}

ModInfo::ModInfo(std::string name)
    : mNameInternal{ std::move(name) }
    , mName{ mNameInternal }
{
}

std::string ModInfo::Dump() const
{
    auto json = nlohmann::json::object();
    json["name"] = mName;
    json["description"] = mDescription;
    json["author"] = mAuthor;
    json["version"] = mVersion;

    json["image_map"] = mCustomImages;
    json["bug_fixes"] = mBugFixes;

    return json.dump();
}

void ModInfo::ReadExtendedInfoFromJson(std::string_view path)
{
    try
    {
        if (auto json_file = std::ifstream(std::string{ path }))
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
            get_if_contained(mBugFixes, "bug_fixes");
        }
    }
    catch (const std::exception& e)
    {
        LogError("Failed loading mod_info.json: {}", e.what());
    }
}

void ModInfo::ReadFromDatabase(const ModDatabase& mod_db)
{
    try
    {
        const auto& mod_info = mod_db.GetModInfo();
        if (!mod_info.empty())
        {
            auto json = nlohmann::json::parse(mod_info);

            CustomImages old_custom_images;
            if (json.contains("image_map"))
            {
                json["image_map"].get_to(old_custom_images);
            }

            for (const auto& [old_path, old_image] : old_custom_images)
            {
                auto& new_image = mCustomImages[old_path];
                if (new_image.ImageMap != old_image.ImageMap)
                {
                    new_image.Outdated = true;
                    for (auto& [target_path, image_map] : old_image.ImageMap)
                    {
                        if (!new_image.ImageMap.contains(target_path))
                        {
                            new_image.ImageMap.insert(std::pair{ target_path, CustomImageMap{} });
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        LogError("Failed loading mod .db: {}", e.what());
    }
}
