#pragma once

#include <array>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "util/image.h"

class VirtualFilesystem;

class SpriteSheetMerger {
public:
	SpriteSheetMerger(const class PlaylunkySettings& settings);
	SpriteSheetMerger(const SpriteSheetMerger&) = delete;
	SpriteSheetMerger(SpriteSheetMerger&&) = delete;
	SpriteSheetMerger& operator=(const SpriteSheetMerger&) = delete;
	SpriteSheetMerger& operator=(SpriteSheetMerger&&) = delete;
	~SpriteSheetMerger();

	void GatherSheetData();

	void RegisterSheet(const std::filesystem::path& full_sheet, bool outdated, bool deleted);

	bool NeedsRegeneration(const std::filesystem::path& destination_folder) const;

	bool GenerateRequiredSheets(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder, VirtualFilesystem& vfs);

private:
	struct TargetSheet;
	bool NeedsRegen(const TargetSheet& target_sheet, const std::filesystem::path& destination_folder) const;

	void MakeItemsSheet();
	void MakeJournalItemsSheet();
	void MakeJournalMonstersSheet();
	void MakeJournalPeopleSheet();
	void MakeJournalStickerSheet();
	void MakeMountsTargetSheet();
	void MakePetsTargetSheet();
	void MakeCharacterTargetSheet(std::string_view color);
	void MakeMenuLeaderTargetSheet();

	bool mRandomCharacterSelectEnabled;
	bool mGenerateCharacterJournalStickersEnabled;
	bool mGenerateCharacterJournalEntriesEnabled;
	bool mGenerateStickerPixelArtEnabled;

	struct ImageSize {
		std::uint32_t Width;
		std::uint32_t Height;
	};
	struct Tile {
		std::uint32_t Left;
		std::uint32_t Top;
		std::uint32_t Right;
		std::uint32_t Bottom;
	};
	struct TileMapping {
		Tile SourceTile;
		Tile TargetTile;
	};
	struct SourceSheet {
		std::filesystem::path Path;
		ImageSize Size;
		std::vector<TileMapping> TileMap;
		std::function<Image(Image, ::ImageSize)> Processing;
	};
	struct TargetSheet {
		std::filesystem::path Path;
		ImageSize Size;
		std::vector<SourceSheet> SourceSheets;
		bool RandomSelect;
	};
	std::vector<TargetSheet> m_TargetSheets;

	struct RegisteredSourceSheet {
		std::filesystem::path Path;
		bool Outdated;
		bool Deleted;
	};
	std::vector<RegisteredSourceSheet> m_RegisteredSourceSheets;
};
