# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.13.0] - 2021-01-28

<img src="https://img.shields.io/badge/Spelunky 2-1.25.2-orange">

### Added
- Bugfix for vines that have 0, 3 or 4 connecting vines (thanks Omelette)

### Changed
- Bumped the supported version of Spelunky

## [0.12.0] - 2021-12-18

<img src="https://img.shields.io/badge/Spelunky 2-1.25.0b-orange">

### Added
- Mods can ship with a `savegame.sav` file, users will then load from and save to that file. Can be disabled in `playlunky.ini` via `allow_save_game_mods`, defaults to `on`.
- `block_save_game` to block the game from saving to `savegame.sav`, in case you want to use mods without affecting your save. Set in `playlunky.ini`, defaults to `off`.
- Dev option `enable_sprite_hot_loading` which automatically reloads sprite sheets (vanilla sheets, entity sheets and custom mapping sheets) when they change during runtime. This means you can work on your sheets without restarting the game. Set in `playlunky.ini`, defaults to `off`.
- Dev option `sprite_hot_load_delay` to declare the delay measured in ms between a sheet changing and the game reloading it. Set in `playlunky.ini`, defaults to `400`.
- Reloading custom textures requires a call to `reload_texture` or a call with an identical texture definition to `define_texture` (thus reloading a script mod will likely reload all custom textures).

## [0.11.1] - 2021-11-10

<img src="https://img.shields.io/badge/Spelunky 2-1.25.0b-orange">

### Fixed
- Probably fixed theme-swap bug that was sometimes triggered after playing online.

## [0.11.0] - 2021-10-25

<img src="https://img.shields.io/badge/Spelunky 2-1.25.0b-orange">

### Changed
- Bumped the supported version of Spelunky
- Reenabled script mods

### Added
- An option that allows loading plain `.str` files, disable by default. This option only changes default behavior, it's not adding any feature.

## [0.10.1] - 2021-09-29

<img src="https://img.shields.io/badge/Spelunky 2-1.23.3-orange">

### Changed
- Reenable character name, gender and heart color mods

## [0.10.0] - 2021-09-23

<img src="https://img.shields.io/badge/Spelunky 2-1.23.3-orange">

### Changed
- Bumped the supported version of Spelunky
- Disabled script mods while that is being worked on
- Disable character name, gender and heart color mods
- Some other mods will likely not work either

## [0.9.3] - 2021-09-21

<img src="https://img.shields.io/badge/Spelunky 2-1.21.0c-orange">

### Added
- An unmissable popup when starting up, give us some time

## [0.9.2] - 2021-08-31

<img src="https://img.shields.io/badge/Spelunky 2-1.21.0c-orange">

### Added
- An option to make FMOD update synchronously, this fixes some crashes that can happen with audio mods

### Changed
- Added a Playlunky specific ImGui .ini file

### Fixed
- The cursed audio crash that plagued ancient CPUs is now gone

## [0.9.1] - 2021-08-11

<img src="https://img.shields.io/badge/Spelunky 2-1.21.0c-orange">

### Fixed
- A couple of crashes that could occur when using custom textures (fix from OL)

## [0.9.0] - 2021-08-04

<img src="https://img.shields.io/badge/Spelunky 2-1.21.0c-orange">

### Added
- Lua console, check the Wiki for more details

### Changed
- Latest Lua features from OL
- Comes with a big refactor of the systems backing Lua scripts

### Fixed
- Crackling audio when using cached decoded audio files
- Random issues with custom and modded audio playback

## [0.8.13] - 2021-07-16

<img src="https://img.shields.io/badge/Spelunky 2-1.21.0c-orange">

### Changed
- Just release latest Lua features as stable

## [0.8.12] - 2021-07-11

<img src="https://img.shields.io/badge/Spelunky 2-1.21.0c-orange">

### Fixed
- Sprite sheets starting with `char_` were never converted to be readable by the game, if they are not known files they should be tho

## [0.8.11] - 2021-07-10

<img src="https://img.shields.io/badge/Spelunky 2-1.21.0c-orange">

### Fixed
- Allows using images with weird bit depth, that said, the game wants 8 bits per channel so just save your images with that please
- A bug that prevented script mods from loading the first time the game is started after disabling speedrun mode

## [0.8.10] - 2021-07-02

<img src="https://img.shields.io/badge/Spelunky 2-1.21.0c-orange">

### Fixed
- Another bug with the custom sprite mapping, should be the last one 🤞

## [0.8.9] - 2021-07-01

<img src="https://img.shields.io/badge/Spelunky 2-1.21.0c-orange">

### Fixed
- Revert accidentally released experimental changes

## [0.8.8] - 2021-07-01

<img src="https://img.shields.io/badge/Spelunky 2-1.21.0c-orange">

### Fixed
- Issue with loading images without an alpha channel
- Bug that prevented custom sprite mapping from working

## [0.8.7] - 2021-06-21

<img src="https://img.shields.io/badge/Spelunky 2-1.21.0c-orange">

### Fixed
- A crash when running the daily
- Not correctly auto-recognizing speedrun mode from enabled mods
- Broken heart color read from char_*.json file when the `0x` prefix was not present

## [0.8.6] - 2021-06-20

<img src="https://img.shields.io/badge/Spelunky 2-1.21.0c-orange">

### Added
- Support for more image formats like .webp, .tiff, .bmp and others

## [0.8.5] - 2021-06-18

<img src="https://img.shields.io/badge/Spelunky 2-1.21.0c-orange">

### Added
- Newest features from Overlunky

## [0.8.4] - 2021-06-11

<img src="https://img.shields.io/badge/Spelunky 2-1.21.0c-orange">

### Added
- Newest features from Overlunky
- Character names, heart color and gender control in one json file

### Fixed
- Corrupted audio when source file was not 32 bits per sample

## [0.8.3] - 2021-06-02

<img src="https://img.shields.io/badge/Spelunky 2-1.21.0c-orange">

### Added
- Newest features from Overlunky (texture API)

## [0.8.2] - 2021-05-26

<img src="https://img.shields.io/badge/Spelunky 2-1.21.0c-orange">

### Fixed
- Bug with loose audio mods not loading the loose files

## [0.8.1] - 2021-05-22

<img src="https://img.shields.io/badge/Spelunky 2-1.21.0c-orange">

### Added
- New level gen API to be used from Lua

### Fixed
- Some issues with corrupt custom audio, both from loose file mods as well as completely custom audio
- Fixed some issues with cache invalidation when a mod uses custom tile map
- Fixed generation of load_order.txt when a mod ships mod_info.json

## [0.8.0] - 2021-05-10

<img src="https://img.shields.io/badge/Spelunky 2-1.21.0c-orange">

### Changed
- Bumped the supported version of Spelunky

## [0.7.9] - 2021-05-09

<img src="https://img.shields.io/badge/Spelunky 2-1.20.4d-orange">

### Fixed
- Fixed a bug with callbacks running at wrong

## [0.7.8] - 2021-05-08

<img src="https://img.shields.io/badge/Spelunky 2-1.20.4d-orange">

### Fixed
- Fixed a possible crash with new pet entity sheets while stitching heads

## [0.7.7] - 2021-05-08

<img src="https://img.shields.io/badge/Spelunky 2-1.20.4d-orange">

### Added
- Support for character heart color mods
- Support for pet entity sheets v2

### Changed
- Upscale output sprite sheets based on input entity sheets

### Fixed
- Fixed a bug with upscaled entity sheets

## [0.7.6] - 2021-05-06

<img src="https://img.shields.io/badge/Spelunky 2-1.20.4d-orange">

### Added
- Experimental extended sound API

## [0.7.5] - 2021-05-01

<img src="https://img.shields.io/badge/Spelunky 2-1.20.4d-orange">

### Fixed
- Double messages from scrips

## [0.7.4] - 2021-04-28

<img src="https://img.shields.io/badge/Spelunky 2-1.20.4d-orange">

### Changed
- Bumped Overlunky latest

### Added
- Support for character name changes in Character Select

## [0.7.3] - 2021-04-23

<img src="https://img.shields.io/badge/Spelunky 2-1.20.4d-orange">

### Changed
- Got some unreleased Overlunky features, don't rely on them
- Slightly improved startup times with disabled mods
- Limit the number of messages that can be pushed from Lua

### Fixed
- Issue with modified date on files being read wrong, causing cache to sometimes invalidate without reason
- Load order issues when using both old and new pets/mounts entity sheets at once
- Crash when playing the daily challenge

## [0.7.2] - 2021-04-19

### Changed
- Stronger posterization on sticker pixel art gen
- Scripts now execute according to load order
- Mod folder fixup moves only known texture and audio files
- Make the watermark almost invisible during gameplay
- Made the log output when extracting game assets a bit clearer

### Fixed
- Invalidate cache when sprite settings change
- Bug when loading old mounts entity sheets
- Crash with partially incomplete character sheets
- Wrap script descriptions instead of cutting them off
- Correctly reinitialize scripts when reloading them

### Added
- All the new entity sheets
- Custom sprite sheet mapping
- Warning for unsafe scripts
- Speedrun mode that loads only character mods and has no watermark during gameplay

### Removed
- The need for steam_appid.txt when running Playlunky

## [0.7.1] - 2021-04-02

<img src="https://img.shields.io/badge/Spelunky 2-1.20.4d-orange">

### Changed
- Bumped Overlunky to get some new features

### Fixed
- Issue with cached string tables and plain string table mods
- A bug in playlunky.ini generation

## [0.7.0] - 2021-03-29

<img src="https://img.shields.io/badge/Spelunky 2-1.20.4d-orange">

### Added
- Script support based on Overlunky
- Nightly builds that always stay up to date with Overlunky WHIP
- Support for new character sheets
- Options to control sticker/journal generation
- Option to generate sticker pixel art
- Option to disable loose file warning
- Generate .ini file instead of shipping it

### Changed
- Bumped the supported version of Spelunky
- Moved mod .db into the global Packs/.db folder

### Fixed
- Hopefully made the rendering hooks compatible with Windows 7 (or more generally not Windows 10)
- Correctly handle deleted mods
- Correctly regenerate stickers, journal and characters when modifying a plain character sheet
- Avoid converting texture files that are not read by the game
- Don't leak memory when unzipping a mod
- A typo in a log message
- A crash when no mods were enabled
- Another typo in a log message
- Problem with the installer expecting to be run from a folder called "Spelunky 2"
- Some issues with files that use weird case
- Issue where loose audio would be cached even if the setting was disabled

## [0.6.2] - 2021-03-11

<img src="https://img.shields.io/badge/Spelunky 2-1.20.3c-orange">

### Fixed
- String mods work again now since the removal of strings08.str

## [0.6.1] - 2021-03-11

<img src="https://img.shields.io/badge/Spelunky 2-1.20.3c-orange">

### Fixed
- Removed cursor since it's not needed and interferes with Overlunky

## [0.6.0] - 2021-03-10

<img src="https://img.shields.io/badge/Spelunky 2-1.20.3c-orange">

### Added
- Hooks the games rendering now to add imgui in preparation for script support (which is still ways off)
- On-screen error printouts
- A watermark with version and all

### Changed
- Merged sticker gen with entity merging code, thus full char sheets and regular char sheets work well together
- Make random character select work with full char sheets

### Fixed
- Correctly invalidate combined sprite sheets when updating load_order.txt (might have to be optimized in the future)
- Bad size in turkey sheet
- Bad tile pos for journal entry of hired hand
- Small bug that caused a tiny performance loss during startup

## [0.5.10] - 2021-27-02

<img src="https://img.shields.io/badge/Spelunky 2-1.20.3c-orange">

### Fixed
- Internal issue with mod names that sometimes caused issues with manually editing load_order.txt

## [0.5.9] - 2021-27-02

<img src="https://img.shields.io/badge/Spelunky 2-1.20.3c-orange">

### Fixed
- Issue where Playlunky would move unkown files into the top level folder when the intention was to move files to their known folders.
- A typo in a logging message.

## [0.5.8] - 2021-26-02

<img src="https://img.shields.io/badge/Spelunky 2-1.20.3c-orange">

### Changed
- Bumped the supported version of Spelunky... again...

## [0.5.7] - 2021-25-02

<img src="https://img.shields.io/badge/Spelunky 2-1.20.3a-orange">

### Changed
- Bumped the supported version of Spelunky.

## [0.5.6] - 2021-22-02

<img src="https://img.shields.io/badge/Spelunky 2-1.20.2a-orange">

### Added
- Loose audio file loading for both .wav and .ogg files. Supported formats are .wav, vorbis .ogg, opus .ogg, .flac and .mp3
- Option to determine whether loose audio files should be enabled. This is in order to avoid slow startup times or additional disk usage.
- Option to control whether audio files should be cached in a decoded format or not. This is a tradeoff where enabling this option will result in faster startup times but require additional disk usage.

### Changed
- mod.db version, this will force a regeneration of assets on first load

## [0.5.5] - 2021-17-02

<img src="https://img.shields.io/badge/Spelunky 2-1.20.2a-orange">

### Added
- Version printouts for possibly easier debugging.

### Fixed
- Fixed an issue when a shader mod would modify more than one function.

## [0.5.4] - 2021-07-02

<img src="https://img.shields.io/badge/Spelunky 2-1.20.2a-orange">

### Added
- Now support loading loose .bank files, meaning audio mods will work with Playlunky.

### Fixed
- Fixed an issue with extracting assets that have non-matching hash length.

## [0.5.3] - 2021-04-02

<img src="https://img.shields.io/badge/Spelunky 2-1.20.2a-orange">

### Changed
- Bumped the supported version of Spelunky.

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
- Bumped the supported version of Spelunky.

### Fixed
- Fix a crash when deleting a mod folder that was previously loaded

## [0.5.0] - 2021-27-01

<img src="https://img.shields.io/badge/Spelunky 2-1.20.1c-orange">

### Changed
- Bumped the supported version of Spelunky.

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