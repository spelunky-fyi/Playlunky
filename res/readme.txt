This same readme or an updated version of it can be found online at the [Playlunky Repository](https://github.com/Malacath-92/Playlunky/wiki)

## Disclaimer:
1. You are strongly discouraged from using any modding tools in your actual online Steam installation as to prevent unlocking achievements, corrupting your savefile and cheating in the leaderboards. You should make a copy of your game somewhere else and install  [Mr. Goldbergs Steam Emulator](https://gitlab.com/Mr_Goldberg/goldberg_emulator/-/releases) in the game directory.
2. That being said, you can *probably* use texture mods online in Steam just fine.
3. Always create a backup of your local save file (i.e. savegame.sav) by copying it to a safe location.
4. Backing up your Spel2.exe is not necessary when using Playlunky as it does not modify the executable.
5. This tool is provided as is without any warranties. If do you break anything using it we can not take responsibility.
6. *Do not report modding related bugs to Blitworks.*

# Usage
## Installing Playlunky
1. Download the latest release [here](https://github.com/Malacath-92/Playlunky/releases) after making sure that it is compatible with your installed version of Spelunky 2 (see the upper right corner in the main menu)
2. Extract the downloaded zip file to your Spelunky 2 installation folder, usually `C:\Program Files (x86)\Steam\SteamApps\common\Spelunky 2`.
3. Run `playlunky_launcher.exe` if you want to start the game with mods, run `Spel2.exe` directly or through Steam to disable mods. 

## Installing Mods
1. Download a mod from [spelunky.fyi](https://spelunky.fyi/mods/).
2. Place the contents of the mod into a subfolder in `Spelunky 2/Mods/Packs` (you might need to create these folders if you have no mods installed)
<br>Alternatively you can just place the zipped mod directly into the `Spelunky 2/Mods/Packs` folder.
3. Some mods may require you to rename the files that they ship, so always refer to the instructions that come with the mod.
4. You should now have a folder or a zip inside the Packs folder, named after the mod you downloaded, for example `Spelunky 2/Mods/Packs/Cat Spelunker` or `Spelunky 2/Mods/Packs/Cat Spelunker.zip`.
5. Launch the game through `playlunky_launcher.exe`, this will automatically install your mod. For larger mods (such as `fingerspit's Remix Mod`) the installation may take a while since Playlunky has to convert all files to a format usable by the base game. Give it a few minutes, if nothing seems to happen report a bug.
6. Enjoy your mod every time you launch the game through `playlunky_launcher.exe`.

## Additional Features
To declare what order mods should be loaded in (e.g. when two mods replace the same file from the base game and you prefer one over the other) open the file `Spelunky 2/Mods/Packs/load_order.txt` and just sort the mods in the order you want them to load in. Make sure every line contains exactly one mod name.

Also using `load_order.txt` you can disable mods by prefixing their name with `--`, aka two minus signs.

# Creating Mods
This tool is not sufficient for creating mods. You will instead need [Modlunky](github.com/spelunky-fyi/modlunky2/wiki), follow the instructions in their wiki. 
When developing mods while using Playlunky however you do not need to repack the executable whenever you change your assets (i.e. skip the step "Repacking with Modlunky 2"). Also note that Playlunky will automatically organize files in your mod, meaning it might move files to different folders within your mods directory after you created them. This usually just means it will place textures into `Data/Textures` and levels into `Data/Levels`.
If you are ready to share your mod make sure you delete the ".db" folder in your mod folder before zipping it up.

## Character Mods
When creating a character mod you may opt to add a small sticker (what is found in `journal_stickers.png`) and a journal entry (what is found in `journal_entry_people.png`) for your character. Do this by changing those two files and deleting everything but the sticker/entry you changed or by adding the sticker directly to your `char_***.png` sprite sheets. In the latter case place the graphics to the right of the petting  at the following tiles with the given sizes:
```
Sticker:
    Tile Position:  8x14
    Pixel Position: 1024x1792
    Pixel Size:     80x80`
Journal Entry
    Tile Position:  9x14
    Pixel Position: 1152x1792
    Pixel Size:     160x160` (note this overlaps with three adjacent empty tiles).
```

## Shader Mods
To mod shaders place a `shaders_mod.hlsl` file into your mod folder that contains only the functions that you want to modify. For now it is only supported to modify functions, not add them. Also make sure that you copy the exact same signature from the original `shaders.hlsl` file, Playlunky does not properly parse the code but only matches strings.

## String Mods
_Modding only select strings is fully supported in Playlunky but no extractor exists as of yet that can help in procuring the base hashes of strings._
To mod only select strings add a `strings00_mod.str` file (or the equivalent for any of the other 8 string files) and place for each string you want to change one line in that file of the pattern `[hash of original english string]: [new string]`. Empty lines as well as lines starting with `#` are ignored. An example would be the following `strings00.str` which modifies the strings in the main menu:
```
# Main Menu
0x3f2ef8d8: Deth
0x38b0d081: Deth Together
0x361d77c6: For Dummies
0x45828a5e: How Bad am I
0x9ee2eb06: Meself
0x874447b3: Real Deth
```

# Reporting Bugs or Requesting Features
If you find any bugs that appear related to your usage with Playlunky or it is missing some features that you absolutely need, head to the [Issues](https://github.com/Malacath-92/Playlunky/issues) page, look through the existing issues to see if it has already been reported and if not create an issue. When reporting bugs always attach your games log file, found in the Spelunky 2 installation directory and named `spelunky.log`.