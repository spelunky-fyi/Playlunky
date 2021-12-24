#pragma once

#include <array>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "sprite_sheet_merger_types.h"
#include "util/image.h"

class VirtualFilesystem;
class EntityDataExtractor;

class SpriteSheetMerger
{
  public:
    SpriteSheetMerger(const class PlaylunkySettings& settings);
    SpriteSheetMerger(const SpriteSheetMerger&) = delete;
    SpriteSheetMerger(SpriteSheetMerger&&) = delete;
    SpriteSheetMerger& operator=(const SpriteSheetMerger&) = delete;
    SpriteSheetMerger& operator=(SpriteSheetMerger&&) = delete;
    ~SpriteSheetMerger();

    void GatherSheetData(bool force_regen_char_journal, bool force_regen_char_stickers);

    void RegisterSheet(const std::filesystem::path& full_sheet, bool outdated, bool deleted);
    void RegisterCustomImages(const std::filesystem::path& base_path, const std::filesystem::path& original_data_folder, std::int64_t priority, const CustomImages& custom_images);

    bool NeedsRegeneration(const std::filesystem::path& destination_folder) const;

    bool GenerateRequiredSheets(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder, VirtualFilesystem& vfs, bool force_reload = false);

  private:
    friend class EntityDataExtractor;

    struct TargetSheet;
    bool NeedsRegen(const TargetSheet& target_sheet, const std::filesystem::path& destination_folder) const;

    void MakeItemsSheet();
    void MakeJournalItemsSheet();
    void MakeJournalMonstersSheet();
    void MakeJournalMonstersBigSheet();
    void MakeJournalPeopleSheet(bool force_regen);
    void MakeJournalStickerSheet(bool force_regen);
    void MakeMountsTargetSheet();
    void MakePetsTargetSheet();
    void MakeMonstersTargetSheet();
    void MakeBigMonstersTargetSheet();
    void MakeCharacterTargetSheet(std::string_view color);
    void MakeMenuLeaderTargetSheet();
    void MakeMenuBasicTargetSheet();
    void MakeCaveDecoTargetSheet();

    bool mRandomCharacterSelectEnabled;
    bool mGenerateCharacterJournalStickersEnabled;
    bool mGenerateCharacterJournalEntriesEnabled;
    bool mGenerateStickerPixelArtEnabled;

    struct TargetSheet
    {
        std::filesystem::path Path;
        SheetSize Size;
        std::vector<SourceSheet> SourceSheets;
        std::vector<MultiSourceTile> MultiSourceTiles;
        bool RandomSelect;
        bool ForceRegen;
    };
    std::vector<TargetSheet> m_TargetSheets;

    struct RegisteredSourceSheet
    {
        std::filesystem::path Path;
        bool Outdated;
        bool Deleted;
    };
    std::vector<RegisteredSourceSheet> m_RegisteredSourceSheets;

    std::unique_ptr<EntityDataExtractor> m_EntityDataExtractor;

    struct LoadedImage
    {
        std::filesystem::path ImagePath;
        std::unique_ptr<Image> ImageFile;
    };
    std::vector<LoadedImage> m_CachedImages;
};
