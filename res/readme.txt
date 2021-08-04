This same readme or an updated version of it can be found online at the [Playlunky Repository](https://github.com/Malacath-92/Playlunky/wiki)

## Disclaimer:
1. You are strongly discouraged from using any modding tools in your actual online Steam installation as to prevent unlocking achievements, corrupting your savefile and cheating in the leaderboards. You should make a copy of your game somewhere else and install  [Mr. Goldbergs Steam Emulator](https://gitlab.com/Mr_Goldberg/goldberg_emulator/-/releases) in the game directory.
2. That being said, you can *probably* use texture mods online in Steam just fine.
3. Always create a backup of your local save file (i.e. savegame.sav) by copying it to a safe location.
4. Backing up your Spel2.exe is not necessary when using Playlunky as it does not modify the executable.
5. This tool is provided as is without any warranties. If do you break anything using it we can not take responsibility.
6. *Do not report modding related bugs to Blitworks.*

# Usage

## Playlunky ❤️'s Modlunky
Playlunky is integrated within Modlunky 2 as of version 0.11.0, which means you can download and use Playlunky from an actual user interface. So download Modlunky 2 rather than directly downloading Playlunky. To do so, just download Modlunky, start it up, navigate to the Playlunky tab, select the newest version of Playlunky on the top right, press download. The rest of the interface lets you disable mods, change load order of mods and set settings. See [Additional Features](#additional-features) for a description of those features.

## Installing Playlunky without Modlunky (don't)
1. Download the latest release [here](https://github.com/spelunky-fyi/Playlunky/releases) after making sure that it is compatible with your installed version of Spelunky 2 (see the upper right corner in the main menu). Do not download the source code, you want `palylunky.zip`.
2. Extract the downloaded zip file to your Spelunky 2 installation folder, usually `C:\Program Files (x86)\Steam\SteamApps\common\Spelunky 2`.
3. Run `playlunky_launcher.exe` if you want to start the game with mods, run `Spel2.exe` directly or through Steam to disable mods. 

## Installing Mods
### Via Modlunky
1. Create an account on [spelunky.fyi](https://spelunky.fyi/mods/), navigate to `Settings`, press `Click to reveal` next to `API Token` and copy the red text
2. Open Modlunky, go to the `Settings` tab, Press `Update Token`, paste the previously copied text and press `OK`
3. Navigate to the mod you want to install while Modlunky is open
4. Press `Install Latest` and wait just a few seconds.
### Via the Installer
1. Download a mod from [spelunky.fyi](https://spelunky.fyi/mods/).
2. Drag and drop the downloaded file (might be a `.png` or a `.zip`, it doesn't matter) onto `playlunky_installer.exe` (the one with the blueish icon)
3. Wait for it to say `Installation was successfull...`
4. Enjoy your mod every time you launch the game through `playlunky_launcher.exe`.
### Manually
1. Download a mod from [spelunky.fyi](https://spelunky.fyi/mods/).
2. Place the contents of the mod into a subfolder in `Spelunky 2/Mods/Packs` (you might need to create these folders if you have no mods installed)
<br>Alternatively you can just place the zipped mod directly into the `Spelunky 2/Mods/Packs` folder.
3. Some mods may require you to rename the files that they ship, so always refer to the instructions that come with the mod.
4. You should now have a folder or a zip inside the Packs folder, named after the mod you downloaded, for example `Spelunky 2/Mods/Packs/Cat Spelunker` or `Spelunky 2/Mods/Packs/Cat Spelunker.zip`. Don't put loose files directly into the `Packs` folder, if the mod you downloaded is just a single file just create your own folder with a random name.
5. Launch the game through `playlunky_launcher.exe`, this will automatically install your mod. For larger mods (such as `fingerspit's Remix Mod`) the installation may take a while since Playlunky has to convert all files to a format usable by the base game. Give it a few minutes, if nothing seems to happen report a bug.
6. Enjoy your mod every time you launch the game through `playlunky_launcher.exe`.

## Additional Features

### Load Order of Mods
To declare what order mods should be loaded in (e.g. when two mods replace the same file from the base game and you prefer one over the other) open the file `Spelunky 2/Mods/Packs/load_order.txt` and just sort the mods in the order you want them to load in. Make sure every line contains exactly one mod name.

### Disabling Mods
Also using `load_order.txt` you can disable mods by prefixing their name with `--`, aka two minus signs.

### Playlunky Settings
Next to `playlunky_launcher.exe` you will find `playlunky.ini` which has settings you can turn on and off simply by changing the word after the `=` sign. Some of these settings can also be modified from Modlunky directly. The available categories and their settings are the following.

`[general_settings]`
- `enable_loose_file_warning`: Enables the warning you get when you have loose files in your `Spelunky 2/Packs` folder. This warning is intended for new users to reveal one potential mistake they are making, so it makes sense to disable this when you already know what you are doing.
- `speedrun_mode`: Automatically blocks all mods other than character mods. This means you can enable this if you want to do a speedrun that is legal according to the current rules.
- `disable_caching`: Disables caching of mod data, so every time Playlunky starts all conversions and merging of files will happen as if you just freshly installed your mods.

`[script_settings]`
- `enable_developer_mode`: Allows reloading script mods by pressing `Ctrl`+`F4` anywhere while the game is running and accepting input.
- `enable_developer_console`: Enables the [Lua Console](#lua-console)
- `enable_developer_console`: Defines how many items are saved in the console history

`[audio_settings]`
- `enable_loose_audio_files`: If enabled allows Playlunky to load loose audio files from mods rather than just loading full soundbank files. Some mods require this setting to be enabled in order to get the full experience. The only reason this is an option is because it will incur a few more seconds of startup time.
- `cache_decoded_audio_files`: If enabled Playlunky will cache decoded loose audio files, which will speed up startup time after first load but requires more space on your hard drive.

`[sprite_settings]`
- `random_character_select`: If this setting is on, when loading a character file, e.g. `char_black.png`, Playlunky will ignore load order and instead load char_black.png from a random mod, among all mods that ship with this file. Mods that ship char_black_full.png are only supported from v0.6.0 and later.
- `generate_character_journal_stickers`: If enabled a sticker is generated for old style character mods.
- `generate_character_journal_entries`: If enabled a journal sprite is generated for old style character mods.
- `generate_sticker_pixel_art`: If enabled together with `generate_character_journal_stickers` the generated sticker will have a crude pixel art in the style of vanilla stickers.

`[key_bindings]`
- `console`: Button to open the console
- `console_alt`: Alternative button to open the console
- `console_close`: Alternative button to close the console

# Creating Mods
This tool is not sufficient for creating mods. You will instead need [Modlunky 2](github.com/spelunky-fyi/modlunky2/wiki), follow the instructions in their wiki.

When developing mods while using Playlunky however you do not need to repack the executable whenever you change your assets (i.e. skip the step "Repacking with Modlunky 2"). Also note that Playlunky will automatically organize files in your mod, meaning it might move files to different folders within your mods directory after you created them. This usually just means it will place textures into `Data/Textures`, levels into `Data/Levels` and audio into `soundbank`.

Check out [Malacath](https://spelunky.fyi/profile/Malacath/) on spelunky.fyi for a bunch of sample mods for the features that are mentioned below. There are also a number of sample mods for specific scripting features and regularly new ones are uploaded.

If you are ready to share your mod make sure you delete the ".db" folder in your mod folder before zipping it up.

## Mod Properties
You can ship your mod with a `mod.json` file for any extended information about your mod. It is completely optional as some of the information in there is not used yet, but there are features that are only accessible through this file. A bare bones version of this file looks as follows:
```json
{
    "author": "Malacath",
    "name": "Sample Mod",
    "description": "This is a sample mod.",
    "version": "1.0",
}
```

### [Custom Sprite Mapping](https://spelunky.fyi/mods/m/sample-mod-custom-sprite-map/)
If you want to change a sprite for example on `items.png` but you want your mod to be compatible with other mods that change a sprite on the same sheet then you can not just place `items.png` into your mod folder, as that will result in only one mod loading said sprite sheet. For this purpose you will have to use custom sprite mapping, which is a set of instructions for Playlunky to copy regions from an image in your mod into a sprite sheet that the game reads. This is expressed in `mod.json` through an object of the following structure (note that json does not support comments, they are here purely for illustrative purpose):
```js
{
    // author, name, description, version
    "image_map": {
        "path/to/source_image.png": {
            "path/to/target_image.png": [         // Array of mappings
                {
                    "from": [ 0, 0, 512, 128 ],   // Take this region from source_image.png
                    "to": [ 384, 768, 896, 896 ]  // Copy it into this region in target_image.png
                }, 
                {
                    "from": {                     // Can also be expressed as an object for clarity
                        "left": 0,
                        "right": 512,
                        "top": 128,
                        "bottom": 256
                    },
                    "to": {
                        "left": 896,
                        "right": 1048,
                        "top": 768,
                        "bottom": 896
                    }
                }
            ]
        }
    }
}
```
In the example we mentioned above `"path/to/source_image.png"` would be a path to a mod in your mod folder, e.g. `resources/black_vlads_wings.png`, and `"path/to/target_image.png"` would be `"Data/Textures/items.png"`. The regions given should be understood such that they define a box with `(top, left)` being the exact pixel position of the top-left corner of the region and `(bottom, right)` being the exact pixel position of the bottom-right of the region. So selecting a region in your image editor should give you those coordinates directly, just try to stay grid-aligned in order to not overwrite other mods.

If multiple mods write to the same region in the same image then load order will determine which mod gets to final say.

## Audio Mods
As of version 0.8.0 Modlunky 2 extracts the games audio files into the `soundbank` folder, where you will find two folders `ogg` and `wav`. Generally speaking the `wav` folder contains the games SFX and the `ogg` folder contains the games dynamic music snippets.
From version 0.5.6 of Playlunky it is not necessary anymore to repack these assets into a `.bank` file when modding them, instead you can just place the audio files you changed into your mod folder and run directly with Playlunky. Note that the game still decides in most instances how long a clip is played, thus adding clips longer than the original may cut them off in game.
You can also ship your mod with different types of audio files than just `.wav` and `.ogg`, only the actual name of the file matters for resolving it correctly. Supported formats are all those that are supported by [libnyquist](https://github.com/ddiakopoulos/libnyquist#format-support) (`.wav`, `.ogg`, `.opus`, `.wv`, `.mp3`, `.flac`, `.mpc` and `.mpp`) It is advisable not to ship your mods with music files in the `.wav` format because they will be quite large.

Loading audio files requires Playlunky to do a specific preprocessing step that takes a bit more time than the usual startup (~1s), additionally each audio file has to be decoded into memory which requires additional time (10ms-200ms, depending on file type) and memory (50KB-16MB, depending on file type and length). If you do not want to incur this overhead, maybe because you are extra impatient or have only a tiny amount of memory, there is an option provided in `playlunky.ini` that goes by the name of `enable_loose_audio_files`. If you are only extra impatient you can enable the option `cache_decoded_audio_files` which will save all decoded audio files in a raw format and thus speed up loading audio significantly.

## [Sprite Mods using Entity Sprite Sheets](https://spelunky.fyi/mods/m/sample-mod-entity-sheets/)
Modlunky2 has the option to generate entity sprite sheets during the extraction process, look for a checkbox labeled "Create Entity Sprites". These sprite sheets will be placed in `Data/Textures/Entities`. Those sprite sheets contain all sprites used by one entity (i.e. stickers, journal entries, animations) and no other sprites. For example `Mounts/turkey.png` contains the turkey animations, the turkey journal image, the cooked turkey in-game sprite and the cooked turkey journal image. Compare this to `mounts.png` which contains all mounts and none of the extra sprites.

You can modify these entity sprite sheets like any regular sprite sheet and place them into your mod, Playlunky will then automatically generate the files the game requires (e.g. in the case of a modded turkey it will generate `journal_entry_mons.png`, `journal_entry_items.png`, `items.png` and `mounts.png`). This generation takes all enabled mods into account and respects specified load order.

The reason for those entity sprite sheets existence is simply to allow modding entities in a more fine grained way. A mod could previously not modify the turkey without also blocking other mods from modifying Qilin. At this point only very few monsters and NPCs are not available through these sheets.

The `_grid.png` files that are generated next to the entity sheets can be used a guides to make sure sprites stay within their allotted space.

## Character Mods
### Using Entity Sprites
See [Sprite Mods using Entity Sprite Sheets](#sprite-mods-using-entity-sprite-sheets), with regular character sprite sheets users get a generated sticker and journal entry.

### [Character Property Mods](https://spelunky.fyi/mods/m/sample-mod-the-full-character-mod-experience/)
In addition to the sprite sheets characters have other properties such as their name, gender and color. Those have to be modded in a novel way which Playlunky introduced via a `.json` file.

To do this for your sprite mod `char_{color}.png` create a file `char_{color}.json` and fill it with the following json data:
```json
{
    "full_name": "Extra Pink Pilot",
    "short_name": "EP Pilot",
    "color": "0xFF307B",
    "gender": "female"
}
```
Now change the values to the right of the colon on each line to your desired values.

#### Full Name
This can be a maximum of 24 characters, anything else will be truncated.
#### Short Name
This is the name appearing in character select. It can be a maximum of 12 characters, anything else will be truncated.
#### Color
This determines the color of the heart during gameplay as well as the color of constellations. It can be of several formats, either a hex-code as in the example or a json array of floats `"color": [ 0.8, 0.3, 0.4 ]` or lastly a json object containing floats `"color": { "red": 0.8, "green": 0.3, "blue": 0.4 }`. Pick whatever you prefer, none of the three options is superior.
#### Gender
This affects some dialogue in the game, for example being referred to as King or Queen. This is either `"female"` or `"male"`, anything that can not be recognized will fall back to `"female"`. The game does not support any other genders but we support them in spirit  💓 

## [Shader Mods](https://spelunky.fyi/mods/m/sample-mod-shader-function/)
To mod shaders place a `shaders_mod.hlsl` file into your mod folder that contains only the functions that you want to modify. For now it is only supported to modify functions, not add them. Also make sure that you copy the exact same signature from the original `shaders.hlsl` file, Playlunky does not properly parse the code but only matches strings. An example would be this `shaders_mod.hsls` which inverts all colors in-game (thanks @Dregu):
```hlsl
float3 Grading_Helper(float3 color, Texture2D LUT) {
	float slice;
	float sliceLerp = modf(color.b * (LUT_SIZE - 1.f), slice);
	float3 sliceColor1 = LUT.SampleLevel(smoothClampingSamplerState, LUT_Helper(color.r, color.g, slice + 0.f), 0).rgb;
	float3 sliceColor2 = LUT.SampleLevel(smoothClampingSamplerState, LUT_Helper(color.r, color.g, slice + 1.f), 0).rgb;

	// The next line was changed to return 1.f - color
	return 1.f - lerp(sliceColor1, sliceColor2, sliceLerp);
}
```

## [String Mods](https://spelunky.fyi/mods/m/sample-mod-custom-strings/)

To mod only select strings first extract your exe with [Modlunky](https://github.com/spelunky-fyi/modlunky2/releases) v0.6.4 or higher. Then find inside the Extracted folder a file named `strings00_hashed.str`. Add a `strings00_mod.str` file to your mod folder (or the equivalent for any of the other 7 valid string files) and copy for each string you want to change the corresponding line from `strings00_hashed.str` into your file. Now change only the part after the `:` for each line while leaving the hex code at the beginning of the line intact. Empty lines as well as lines starting with `#` are ignored. An example would be the following `strings00.str` which modifies the strings in the main menu:
```
# Main Menu
0x49054a7e: Deth
0xa1023681: Deth Together
0xfe203cc2: For Dummies
0x34906f28: How Bad am I
0x8ca20866: Meself
0x9a19dfba: Real Deth
```
When translating a mod to another language you can just copy the whole `strings00_mod.str` file, change the number on it to the corresponding language and change the strings. The hex code at the beginning of the line is the same for each language.

## [Script Mods](https://spelunky.fyi/mods/m/remote-bomb-powerup/)

As of v0.7.0 Playlunky supports the same scripts that Overlunky supports. This functionality is still in development and subject to break. Playlunky will also not always be up to date with the latest and greatest Overlunky Whip builds, so it is recommended to develop scripts with Overlunky and only release them for Playlunky use when those features make it into Playlunky, or instruct your users to use Overlunky as well.

Currently only one script per mod pack is loaded, it has to be named `main.lua` and can't require any other scripts. Other than that the same restrictions and API apply as does for Overlunky. Only mods that are enabled will have their script loaded. A settings menu will be open while in the main menu, it can also be opened anywhere else by pressing Ctrl + F4. If you want to use Playlunky for developing scripts you can also enable developer mode in `playlunky.ini` which enables reloading scripts by pressing Ctrl + F5. Errors will be displayed along other errors in the top left corner of the screen.

Check the [Lua API Documentation](https://github.com/spelunky-fyi/overlunky/blob/main/docs/script-api.md) for available functionality.

## Lua Console

As of v0.9.0 Playlunky contains the same console as Overlunky. Inside the console you can write and execute any Lua code that you would be able to execute in a Script Mod and Script Mods can additionally export functionality to the console via [`register_console_command`](https://github.com/spelunky-fyi/overlunky/blob/main/docs/script-api.md#register_console_command). The following controls are available:
- Enable in `playlunky.ini`
- Open with `~` or `\` for US keyboards, if you don't have a US keyboard just try some buttons (configurable in `playlunky.ini`)
- Close with those buttons or `ESC` (configurable in `playlunky.ini`)
- Up & Down Arrow for history, max 20 items in history (configurable in `playlunky.ini`)
- Tab for autocompletion and completion suggestions
- Ctrl + Enter for new-line
- Enter to execute code

# Reporting Bugs or Requesting Features
If you find any bugs that appear related to your usage with Playlunky or it is missing some features that you absolutely need, head to the [Issues](https://github.com/spelunky-fyi/Playlunky/issues) page, look through the existing issues to see if it has already been reported and if not create an issue. When reporting bugs always attach your games log file, found in the Spelunky 2 installation directory and named `spelunky.log`.