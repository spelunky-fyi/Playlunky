# Playlunky
<p align="center">
  <img src="https://img.shields.io/badge/Spelunky 2-1.20.0j-orange">
  <a href="https://isocpp.org/">
      <img src="https://img.shields.io/badge/language-C%2B%2B20-blue.svg">
  </a>
  <a href="https://ci.appveyor.com/project/Malacath-92/playlunky">
      <img src="https://ci.appveyor.com/api/projects/status/yhlybe62omlpbxj3?svg=true">
  </a>
  <a href="https://github.com/Malacath-92/playlunky/actions">
      <img src="https://github.com/Malacath-92/playlunky/workflows/Github%20Actions%20CI/badge.svg">
  </a>
  <a href="https://opensource.org/licenses/MIT" >
      <img src="https://img.shields.io/apm/l/vim-mode.svg">
  </a>
</p>

This is mainly a personal playground to learn the tricks and trades of video game reverse engineering/hacking. It currently is only a wrapper around launching Spelunky 2 with an injected dll which loads resources from disk and injects ScyllaHide but hopefully will become more in the future.

## Build

Only Windows build is supported:
```sh
git clone git@github.com:Malacath-92/Playlunky.git
cd playlunky
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
This project is currently compatible with 1.19.8c
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
    * On first load all png files are automatically converted to dds, so mods can be distributed as usual (mods store a small database to catch when they get updated)