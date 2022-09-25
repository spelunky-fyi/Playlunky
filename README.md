# Playlunky
<p align="center">
    <a href="https://isocpp.org/">
        <img src="https://img.shields.io/badge/language-C%2B%2B20-blue.svg">
    </a>
    <a href="https://ci.appveyor.com/project/ZeroCostGoods/playlunky">
        <img src="https://ci.appveyor.com/api/projects/status/3kobi9p0n277q9qd/branch/main?svg=true">
    </a>
    <a href="https://github.com/spelunky-fyi/playlunky/actions">
        <img src="https://github.com/spelunky-fyi/playlunky/workflows/Github%20Actions%20CI/badge.svg">
    </a>
    <a href="https://github.com/spelunky-fyi/playlunky/actions">
        <img src="https://github.com/spelunky-fyi/playlunky/workflows/Formatting/badge.svg">
    </a>
    <a href="https://opensource.org/licenses/MIT" >
        <img src="https://img.shields.io/apm/l/vim-mode.svg">
    </a>
    <br>
    <img src="https://img.shields.io/badge/Stable: Spelunky 2-1.25.2-orange">
    <img src="https://img.shields.io/badge/Nightly: Spelunky 2-1.27-orange">
</p>

This is a launcher for Spelunky 2 that injects a dll for extended mod management. It currently supports string, shader, sprite and string mods, with some extended features for more fine grained modding. See the [Wiki](https://github.com/spelunky-fyi/Playlunky/wiki) for details.

## Features
- Loose loading of resources
- Mod Management
    - Sprite mods
    - Shader mods
    - String mods
    - Script mods
For more details on each feature check the [Wiki](https://github.com/spelunky-fyi/Playlunky/wiki).

### Development Features
- Anti-anti-debug injection via ScyllaHide
- Logging to console via `--console`

## Credits
A huge thanks to the [spelunky-fyi](https://github.com/spelunky-fyi) team for their input, suggestions, support and for making all their hard work open source. Special thanks to `gmjosack`, `Dregu` and `iojonmbnmb` that made it possible for this tool to exist. And even huger thanks to `SK83RJOSH` for basically teaching me how to reverse engineer software by himself.

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

### Requirements
- MSVC 2019 (for C++20)
- python
    - cmake
    - conan
- clang-format

### Debugging with Visual Studio
If you have installed Spelunky 2 then the install folder should be found during configuration of the project. When CMake can't find the installation directory please make an issue explaining your setup. In that case or when you have a copy of the game outside the actual installation directory that you want to work with you can pass the directory to CMake during configure:
```sh
cmake .. -DSPELUNKY_INSTALL_DIR='C:/Path/To/Install/Folder/Spelunky 2'
cmake --build . --config Release
```
Now the Visual Studio solution should be setup to start the game with `playlunky64.dll` injected when starting a debugging session.

If you want to debug the game itself as well as the `playlunky64.dll` it is highly recommended to get the [Microsoft Child Process Debugging Power Tool](https://marketplace.visualstudio.com/items?itemName=vsdbgplat.MicrosoftChildProcessDebuggingPowerTool) extension and enable child process debugging in `Debug` &rarr; `Other Debug Targets` &rarr; `Child Process Debugging Settings...`

#### Anti-Debugging Prevention
This section can be ignored for Spelunky 2 versions newer than 1.20.0j, perhaps even some older versions.
If the game employs any anti-debugging strategies it is supported to inject ScyllaHide into the game process. To do this download [ScyllaHide from Github](https://github.com/x64dbg/ScyllaHide/tags) and extract it into a ScyllaHide subfolder right next to the game. The `playlunky64.dll` will do the rest.
