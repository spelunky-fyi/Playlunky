#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

class VirtualFilesystem;

class CharacterStickerGenerator {
public:
	CharacterStickerGenerator() = default;
	CharacterStickerGenerator(const CharacterStickerGenerator&) = delete;
	CharacterStickerGenerator(CharacterStickerGenerator&&) = delete;
	CharacterStickerGenerator& operator=(const CharacterStickerGenerator&) = delete;
	CharacterStickerGenerator& operator=(CharacterStickerGenerator&&) = delete;
	~CharacterStickerGenerator() = default;

	bool RegisterCharacter(std::string_view character_color, bool outdated);

	bool NeedsRegeneration() const { return true; }

	bool GenerateStickers(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder,
		const std::filesystem::path& sticker_file, const std::filesystem::path& journal_file, VirtualFilesystem& vfs);

private:
	struct CharacterInfo {
		std::string_view Color;

		struct Index {
			std::uint32_t x;
			std::uint32_t y;
		};
		Index TileIndex;
	};
	std::array<CharacterInfo, 20> mInfos{
		CharacterInfo{ .Color = "yellow", .TileIndex = { 0, 0 } },
		CharacterInfo{ .Color = "magenta", .TileIndex = { 1, 0 } },
		CharacterInfo{ .Color = "cyan", .TileIndex = { 2, 0 } },
		CharacterInfo{ .Color = "black", .TileIndex = { 3, 0 } },
		CharacterInfo{ .Color = "cinnabar", .TileIndex = { 4, 0 } },
		CharacterInfo{ .Color = "green", .TileIndex = { 5, 0 } },
		CharacterInfo{ .Color = "olive", .TileIndex = { 6, 0 } },
		CharacterInfo{ .Color = "white", .TileIndex = { 7, 0 } },
		CharacterInfo{ .Color = "cerulean", .TileIndex = { 8, 0 } },
		CharacterInfo{ .Color = "blue", .TileIndex = { 9, 0 } },
		CharacterInfo{ .Color = "lime", .TileIndex = { 0, 1 } },
		CharacterInfo{ .Color = "lemon", .TileIndex = { 1, 1 } },
		CharacterInfo{ .Color = "iris", .TileIndex = { 2, 1 } },
		CharacterInfo{ .Color = "gold", .TileIndex = { 3, 1 } },
		CharacterInfo{ .Color = "red", .TileIndex = { 4, 1 } },
		CharacterInfo{ .Color = "pink", .TileIndex = { 5, 1 } },
		CharacterInfo{ .Color = "violet", .TileIndex = { 6, 1 } },
		CharacterInfo{ .Color = "gray", .TileIndex = { 7, 1 } },
		CharacterInfo{ .Color = "khaki", .TileIndex = { 8, 1 } },
		CharacterInfo{ .Color = "orange", .TileIndex = { 9, 1 } }
	};

	bool mNeedsRegen{ false };
	std::vector<std::string_view> mModdedCharacters;
};