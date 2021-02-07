# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.5.4] - 2021-07-02

<img src="https://img.shields.io/badge/Spelunky 2-1.20.2a-orange">

### Added
- Now support loading loose .bank files, meaning audio mods will work with Playlunky.

### Fixed
- Fixed an issue with extracting assets that have non-matching hash length.

## [0.5.3] - 2021-04-02

<img src="https://img.shields.io/badge/Spelunky 2-1.20.2a-orange">

### Changed
- Bumped the supported version of Spelunky

## [0.5.2] - 2021-01-02

<img src="https://img.shields.io/badge/Spelunky 2-1.20.1d-orange">

### Fixed
- Removed an erroneous double occurrence of lime, resulting in wrong sticker generation.

## [0.5.1] - 2021-31-01

<img src="https://img.shields.io/badge/Spelunky 2-1.20.1d-orange">

### Added
- Add a playlunky.ini file for future configuration.
- Add support for Modlunky full sheets, e.g. char_black_full.png or poochie_full.png, for pets, mounts and characters.
- Add random mode for character mods (configured in playlunky.ini), which picks a random character mod from all installed mods per color.

### Changed
- Bumped the supported version of Spelunky

### Fixed
- Fix a crash when deleting a mod folder that was previously loaded

## [0.5.0] - 2021-27-01

<img src="https://img.shields.io/badge/Spelunky 2-1.20.1c-orange">

### Changed
- Bumped the supported version of Spelunky

## [0.4.4] - 2021-22-01

<img src="https://img.shields.io/badge/Spelunky 2-1.20.0j-orange">

### Added
- Add a mod installer that should make it super easy to install mods now (also works for Modlunky2 since it loads mods from the same folder)

### Changed
- Shader mods now copy all parts that are not known into the shader (instead of being ignored)
- Shader mods are also properly parsed for comments (single line '//' and multiline '/*' comments should work)
- Slightly optimized pattern lookup in Spel2.exe
- Bumped OpenCV to latest

### Fixed
- Fix a mods folder structure unconditionally, which resolves some rare errors when changing existing mods
- Fixed an issue with pattern lookup in Spel2.exe that wouldn't look for the whole pattern if it contained null-characters

## [0.4.3] - 2021-04-01

<img src="https://img.shields.io/badge/Spelunky 2-1.20.0j-orange">

### Changed
- Add support for (relatively) arbitrary character sheet sizes for sticker generation

## [0.4.2] - 2021-03-01

<img src="https://img.shields.io/badge/Spelunky 2-1.20.0j-orange">

### Changed
- Trim strings before hashing, which should make this release compatible with Modlunky2

## [0.4.1] - 2021-02-01

<img src="https://img.shields.io/badge/Spelunky 2-1.20.0j-orange">

### Changed
- Switched from stl hash to crc32, which should make this release compatible with Modlunky2

## [0.4.0] - 2021-01-01

<img src="https://img.shields.io/badge/Spelunky 2-1.20.0j-orange">

### Added
- Preliminary support for sparse string modding
- Support for sparse shader modding (i.e. placing a `shaders_mod.hlsl` into your mod that contains only modded functions)
- Correctly regenerate stickers and shaders when mods get enabled or disabled

### Changed
- mod.db version, this will force a regeneration of assets on first load
- Always show console on startup so users have some feedback while mods are loaded

### Fixed
- Issue that caused character stickers to be generated every time the game starts
- A rare crash with some of the games logging messages
- Potential graphical errors during character sticker generation

## [0.3.0] - 2020-29-12

<img src="https://img.shields.io/badge/Spelunky 2-1.20.0j-orange">

### Added
- Asset extraction for making future features more feasible (thanks to Modlunky team for their implementation)
- Automatic sticker and journal generation for character mods (thanks @Dregu)

### Changed
- mod.db format for better compatibility with future versions

### Fixed
- Issue of mismatching file extension for DDS files extracted from the game executable
- Issue that cause the load order to be inverted
- Delete converted assets from the .db folder when their source assets were deleted

## [0.2.0] - 2020-26-12

<img src="https://img.shields.io/badge/Spelunky 2-1.20.0j-orange">

### Added
- Automatic mod reorganization, all files in a mod will be placed into their appropriate folders on first load
- Ability to disable mods in `load_order.txt`.

### Removed
- File lookup at root, since mods are now organized correctly instead

### Fixed
- Issue with mods that have spaces in their names

## [0.1.1] - 2020-25-12

<img src="https://img.shields.io/badge/Spelunky 2-1.20.0j-orange">

### Fixed
- Error in readme.txt

## [0.1.0] - 2020-25-12

<img src="https://img.shields.io/badge/Spelunky 2-1.20.0j-orange">

### Added
- Automatic unzipping of zipped mods in the Mods/Packs folder
- Error popup on version mismatch
- A readme.txt with usage instructions

### Changed
- Bumped supported version to 1.20.0j
- File lookup at the root when proper folder structure not present (to align with Modlunky)

### Fixed
- Heap corruption when loading loose files (because of 24 missing bytes in the allocation)

## [0.0.1] - 2020-23-12

<img src="https://img.shields.io/badge/Spelunky 2-1.19.8c-orange">

### Added
- Mod-Management based on the folder structure of Modlunky
- Automatic png to dds conversion
- Loose file loading