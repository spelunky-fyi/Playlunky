#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

#include "sprite_sheet_merger_types.h"

class EntityDataExtractor
{
  public:
    EntityDataExtractor() = default;
    EntityDataExtractor(const EntityDataExtractor&) = delete;
    EntityDataExtractor(EntityDataExtractor&&) = delete;
    EntityDataExtractor& operator=(const EntityDataExtractor&) = delete;
    EntityDataExtractor& operator=(EntityDataExtractor&&) = delete;
    ~EntityDataExtractor() = default;

    void PreloadEntityMappings();

    std::optional<SourceSheet> GetEntitySourceSheet(std::string_view entity_sheet) const;
    std::optional<SourceSheet> GetAdditionalMapping(std::string_view entity_sheet, Tile relative_source_tile, Tile target_tile) const;

  private:
    struct EntityMapping
    {
        std::string_view EntityPath;
        SourceSheet SourceSheet;
        std::uint32_t SourceHeight;
    };
    std::vector<EntityMapping> m_EntityMapping;
};
