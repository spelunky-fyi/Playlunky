# Playlunky

This is mainly a personal playground to learn the tricks and trades of video game reverse engineering. It currently is only a wrapper around launching Spelunky 2 with an injected dll which loads resources from disk but hopefully will become more in the future.

## Build

Only Windows build is supported:
```sh
git clone git@github.com:Malacath-92/Playlunky.git
cd Playlunky
mkdir build
cd build
cmake ..
cmake --build . --config Release
cmake --install .
```
Then take all the build artifacts from the publish folder and paste them into your Spelunky 2 folder, from there you can launch `playlunky_launcher.exe` 
to launch the game with the dll injected.

## Requirements
- MSVC 2019 (for C++20)
- python
    - cmake
    - conan