# Playlunky
<p align="center">
  <img src="https://img.shields.io/badge/Spelunky 2-1.20.0j-orange">
  <a href="https://isocpp.org/">
      <img src="https://img.shields.io/badge/language-C%2B%2B20-blue.svg">
  </a>
  <a href="https://ci.appveyor.com/project/ZeroCostGoods/playlunky">
      <img src="https://ci.appveyor.com/api/projects/status/3kobi9p0n277q9qd/branch/main?svg=true">
  </a>
  <a href="https://github.com/spelunky-fyi/playlunky/actions">
      <img src="https://github.com/spelunky-fyi/playlunky/workflows/Github%20Actions%20CI/badge.svg">
  </a>
  <a href="https://opensource.org/licenses/MIT" >
      <img src="https://img.shields.io/apm/l/vim-mode.svg">
  </a>
</p>

This is mainly a personal playground to learn the tricks and trades of video game reverse engineering/hacking. It currently is only a wrapper around launching Spelunky 2 with an injected dll which loads resources from disk and injects ScyllaHide but hopefully will become more in the future.

## Credits
A huge thanks to the [modlunky2](https://github.com/spelunky-fyi/modlunky2) team for their input, suggestions, support and for making all their hard work open source. Special thanks to `gmosjack`, `Dregu` and `iojonmbnmb` that made it possible for this tool to exist.

## Build

Only Windows build is supported:
```sh
git clone git@github.com:spelunky-fyi/Playlunky.git
cd playlunky
git submodule update --init --recursive
mkdir build
cd build
cmake ..
cmake --build . --config Release
cmake --install .
```
Build artifacts are found in the `publish` folder.

### Debugging
If you have installed Spelunky 2 then the install folder should be found during configuration of the project and starting a debugging session will launch Spelunky 2 with the `playlunky64.dll` injected. If you want to debug the game itself as well as the `playlunky64.dll` it is highly recommended to get the [Microsoft Child Process Debugging Power Tool](https://marketplace.visualstudio.com/items?itemName=vsdbgplat.MicrosoftChildProcessDebuggingPowerTool) extension and enable child process debugging in `Debug` &rarr; `Other Debug Targets` &rarr; `Child Process Debugging Settings...`

Furthermore, if the game employs any anti-debugging strategies it is supported to inject ScyllaHide into the game process. To do this download [ScyllaHide from Github](https://github.com/x64dbg/ScyllaHide/tags) and extract it into a ScyllaHide subfolder right next to the game. The `playlunky64.dll` will do the rest.

### Requirements
- MSVC 2019 (for C++20)
- python
    - cmake
    - conan

## Usage
Copy all build artifacts into your Spelunky 2 folder, from there you can launch `playlunky_launcher.exe` to launch the game with the dll injected.

## Features
* Loose loading of resources    
    * The game still loads files packed in the exe
    * Prioritizes loose files over files packed in the exe
* Anti-anti-debug injection
    * If ScyllaHide exists next to the exe it will inject that to circumvent any anti-debugging measures that might exist in the exe (there appear to be none in this version, but there were in pervious versions)
    * Only tries to inject ScyllaHide if a debugger is attached to the exe on boot
* Logging to console
    * Creates a console window if started with `--console` that grabs all of the games logging output for easier debugging 
* Mod Management
    * Loads mods from the `Mods/Packs` folder, where each folder is one mod
    * Users can specify priority of mods using the `Mods/Packs/load_order.txt` file (useful in case multiple mods replace the same asset)
    * Settings are specified in playlunky.ini
        * Random Character Select: Everytime the game is launched for each character a random sheet will be loaded out of all installed and enabled mods. For example if you installed 5 mods that all change Roffy you will get a random version of Roffy every time you launch the game. Note that this only changes the visuals, not any strings and also does not currently work for full entity mods of characters.
    * On first load:
        * zip files get extracted into their own mod folders
        * the files in the mod folder are reorganized to align with the games original structure
        * entity sprite sheets (as provided by Modlunky in the form of `Entities/*_full.png`) are merged into their respective sheets for the game
        * all other png files are automatically converted to dds, so mods can be distributed as usual (mods store a small database to catch when they get updated)
        * additionally stickers and journal entries are generated from character mods
