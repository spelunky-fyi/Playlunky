# Playlunky
<p align="center">
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

This is mainly a personal playground to learn the tricks and trades of video game reverse engineering/hacking. It currently is only a wrapper around launching Spelunky 2 with an injected dll which loads resources from disk but hopefully will become more in the future.

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
    * If ScyllaHide exists next to the exe it will inject that to circumvent any anti-debugging measures that might exist in the exe (there appear to be none in version 1.9.18c, but there were in pervious versions)
    * Only tries to inject ScyllaHide if a debugger is attached to the exe on boot
* Logging to console
    * Creates a console window if started with `--console` that grabs all of the games logging output for easier debugging 

### Coming Eventually
* Mod Management
    * Loads mods from the `Mods` folder, where each folder is one mod
    * Users can specify priority of mods (in case multiple mods replace the same asset)
    * On first load, all files not compatible with the game will be converted to be compatible (e.g. png &rarr; dds)
    * Mods store a database so that the conversions are redone when a mod gets updated