# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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