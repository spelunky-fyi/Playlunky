#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

class VirtualFilesystem;

class SpriteSheetMerger {
public:
	SpriteSheetMerger(const class PlaylunkySettings& settings);
	SpriteSheetMerger(const SpriteSheetMerger&) = delete;
	SpriteSheetMerger(SpriteSheetMerger&&) = delete;
	SpriteSheetMerger& operator=(const SpriteSheetMerger&) = delete;
	SpriteSheetMerger& operator=(SpriteSheetMerger&&) = delete;
	~SpriteSheetMerger() = default;

	void RegisterSheet(const std::filesystem::path& full_sheet, bool outdated, bool deleted);

	bool NeedsRegeneration(const std::filesystem::path& destination_folder) const;

	bool GenerateRequiredSheets(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder, VirtualFilesystem& vfs);

private:
	struct TargetSheet;
	bool NeedsRegen(const TargetSheet& target_sheet, const std::filesystem::path& destination_folder) const;

	static TargetSheet MakeItemsSheet();
	static TargetSheet MakeJournalItemsSheet();
	static TargetSheet MakeJournalMonstersSheet();
	static TargetSheet MakeJournalPeopleSheet(bool random_character_select_enabled);
	static TargetSheet MakeJournalStickerSheet(bool random_character_select_enabled);
	static TargetSheet MakeMountsTargetSheet();
	static TargetSheet MakePetsTargetSheet();
	static TargetSheet MakeCharacterTargetSheet(std::string_view color, bool random_character_select_enabled);

	bool mRandomCharacterSelectEnabled;

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
