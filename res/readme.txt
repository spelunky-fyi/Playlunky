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
Next to `playlunky_launcher.exe` you will find `playlunky.ini` which has settings you can turn on and off simply by changing the word after the `=` sign. The available settings are the following.
- random_character_select: If this setting is on, when loading a character file, e.g. `char_black.png`, Playlunky will ignore load order and instead load char_black.png from a random mod, among all mods that ship with this file. Mods that ship char_black_full.png are only supported from v0.6.0 and later.
- enable_loose_audio_files: If enabled allows Playlunky to load loose audio files from mods rather than just loading full soundbank files. Some mods require this setting to be enabled in order to get the full experience. The only reason this is an option is because it will incur a few more seconds of startup time.
- cache_decoded_audio_files: If enabled Playlunky will cache decoded loose audio files, which will speed up startup time after first load but requires more space on your hard drive.

# Creating Mods
This tool is not sufficient for creating mods. You will instead need [Modlunky 2](github.com/spelunky-fyi/modlunky2/wiki), follow the instructions in their wiki. 
When developing mods while using Playlunky however you do not need to repack the executable whenever you change your assets (i.e. skip the step "Repacking with Modlunky 2"). Also note that Playlunky will automatically organize files in your mod, meaning it might move files to different folders within your mods directory after you created them. This usually just means it will place textures into `Data/Textures`, levels into `Data/Levels` and audio into `soundbank`.
If you are ready to share your mod make sure you delete the ".db" folder in your mod folder before zipping it up.

## Audio Mods
As of version 0.8.0 Modlunky 2 extracts the games audio files into the `soundbank` folder, where you will find two folders `ogg` and `wav`. Generally speaking the `wav` folder contains the games SFX and the `ogg` folder contains the games dynamic music snippets.
From version 0.5.6 of Playlunky it is not necessary anymore to repack these assets into a `.bank` file when modding them, instead you can just place the audio files you changed into your mod folder and run directly with Playlunky. Note that the game still decides in most instances how long a clip is played, thus adding clips longer than the original may cut them off in game.
You can also ship your mod with different types of audio files than just `.wav` and `.ogg`, only the actual name of the file matters for resolving it correctly. Supported formats are all those that are supported by [libnyquist](https://github.com/ddiakopoulos/libnyquist#format-support) (`.wav`, `.ogg`, `.opus`, `.wv`, `.mp3`, `.flac`, `.mpc` and `.mpp`) It is advisable not to ship your mods with music files in the `.wav` format because they will be quite large.

Loading audio files requires Playlunky to do a specific preprocessing step that takes a bit more time than the usual startup (~1s), additionally each audio file has to be decoded into memory which requires additional time (10ms-200ms, depending on file type) and memory (50KB-16MB, depending on file type and length). If you do not want to incur this overhead, maybe because you are extra impatient or have only a tiny amount of memory, there is an option provided in `playlunky.ini` that goes by the name of `enable_loose_audio_files`. If you are only extra impatient you can enable the option `cache_decoded_audio_files` which will save all decoded audio files in a raw format and thus speed up loading audio significantly.

## Sprite Mods using Entity Sprite Sheets
Modlunky2 has the option to generate entity sprite sheets during the extraction process, look for a checkbox labeled "Create Entity Sprites". These sprite sheets will be placed in `Data/Textures/Entities` and their file names always end in `_full`. Those sprite sheets contain all sprites used by one entity (i.e. stickers, journal entries, animations) and no other sprites. For example `turkey_full.png` contains the turkey animations, the turkey journal image, the cooked turkey in-game sprite and the cooked turkey journal image. Compare this to `mounts.png` which contains all mounts and none of the extra sprites.

You can modify these entity sprite sheets like any regular sprite sheet and place it into your mod, Playlunky will then automatically generate the files the game requires (e.g. in the case of a modded turkey it will generate `journal_entry_mons.png`, `journal_entry_items.png`, `items.png` and `mounts.png`). This generation takes all enabled mods into account and respects specified load order.

The reason for those entity sprite sheets existence is simply to allow modding entities in a more fine grained way. A mod could previously not modify the turkey without also blocking other mods from modifying qilin. This will thus be extended to other monsters and NPCs down the line. For now it is only available for mounts, pets and characters.

The `_grid.png` files that are generated next to the `_full.png` files can be used a guides to make sure sprites stay within their allotted space.

## Character Mods
### Using Entity Sprites
See [Sprite Mods using Entity Sprite Sheets](#sprite-mods-using-entity-sprite-sheets)

### The old Way
When creating a character mod you may opt to add a small sticker (what is found in `journal_stickers.png`) and a journal entry (what is found in `journal_entry_people.png`) for your character. Do this by changing those two files and deleting everything but the sticker/entry you changed or by adding the sticker directly to your `char_***.png` sprite sheets. In the latter case place the graphics to the right of the petting  at the following tiles with the given sizes:
```
Sticker:
    Tile Position:  8x14
    Pixel Position: 1024x1792
    Pixel Size:     128x128 (equals a 1x1 tile)
Journal Entry
    Tile Position:  9x14
    Pixel Position: 1152x1792
    Pixel Size:     256x256 (equals a 2x2 tile)
```

## Shader Mods
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

## String Mods

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


# Reporting Bugs or Requesting Features
If you find any bugs that appear related to your usage with Playlunky or it is missing some features that you absolutely need, head to the [Issues](https://github.com/spelunky-fyi/Playlunky/issues) page, look through the existing issues to see if it has already been reported and if not create an issue. When reporting bugs always attach your games log file, found in the Spelunky 2 installation directory and named `spelunky.log`.