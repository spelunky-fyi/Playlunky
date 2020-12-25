This same readme or an updated version of it can be found online at the [Playlunky Repository](https://github.com/Malacath-92/Playlunky/wiki)

## Disclaimer:
1. You are strongly discouraged from using any modding tools in your actual online Steam installation as to prevent unlocking achievements, corrupting your savefile and cheating in the leaderboards. You should make a copy of your game somewhere else and install [Mr. Goldbergs Steam Emulator](https://gitlab.com/Mr_Goldberg/goldberg_emulator/-/releases) in the game directory.
2. That being said, you can *probably* use texture mods online in Steam just fine.
3. Always create a backup of your local save file (i.e. savegame.sav) by copying it to a safe location.
4. Backing up your Spel2.exe is not necessary when using Playlunky as it does not modify the executable.
5. This tool is provided as is without any warranties. If do you break anything using it we can not take responsibility.
6. *Do not report modding related bugs to Blitworks.*

# Usage
## Installing Playlunky
1. Download the latest release [here](https://github.com/Malacath-92/Playlunky/releases) after making sure that it is compatible with your installed version of Spelunky 2 (see the upper right corner in the main menu)
2. Extract the downloaded zip file to your Spelunky 2 installation folder, usually C:\Program Files (x86)\Steam\SteamApps\common\Spelunky 2
3. Run `playlunky_launcher.exe` if you want to start the game with mods, run `Spel2.exe` directly or through Steam to disable mods. 

## Installing Mods
1. Download a mod from [spelunky.fyi](https://spelunky.fyi/mods/)
2. Place the contents of the mod into a subfolder in `Spelunky 2/Mods/Packs` (you might need to create these folders if you have no mods installed)
3. Some mods may require you to rename the files that they ship, so always refer to the instructions that come with the mod
4. You should now have a folder inside the Packs folder, named after the mod you downloaded, for example `Spelunky 2/Mods/Packs/Cat Spelunker`
5. Launch the game through `playlunky_launcher.exe` to enjoy your mod.

## Creating Mods
This tool is not sufficient for creating mods. You will instead need [Modlunky](github.com/spelunky-fyi/modlunky2/wiki), follow the instructions in their wiki. 
When developing mods while using Playlunky however you do not need to repack the executable whenever you change your assets (i.e. skip the step "Repacking with Modlunky 2").
If you are ready to share your mod make sure you delete the ".db" folder in your mod folder before zipping it up.