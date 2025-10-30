# Arcane Vanguard

A retro-inspired 3D arena RPG prototype built with modern C++ and immediate-mode OpenGL. Command the Vanguard in fast-paced combat against waves of spectral foes and twin boss warlords while a procedural chiptune soundtrack plays in the background.

## Features

* **Title menu** with animated selections for story and training modes.
* **Dynamic arena combat** mixing melee combos, dodges, jumping and mana-powered projectiles.
* **Dozens of enemies** coordinating patrols, chases and ranged assaults plus two multi-phase bosses that unleash bullet-hell patterns when enraged.
* **Diegetic day/night lighting** and floating UI overlays reminiscent of classic 90s RPGs.
* **Procedural background music** generated in real time with a lightweight software synth.

## Controls

| Action | Input |
| --- | --- |
| Move | W/A/S/D |
| Rotate camera | Q / E |
| Dash | Left Shift (while moving) |
| Jump | Space |
| Sword combo | J |
| Arcane volley | K |
| Return to title / Quit | Esc |
| Confirm menu option | Enter |
| Navigate menu | Up / Down |

## Building

This project uses CMake and depends on GLFW, OpenGL, and (on Linux) ALSA for the background music synthesizer. The code targets C++17.

```bash
cmake -S . -B build
cmake --build build
```

On Debian / Ubuntu you can install the required dependencies with your package manager:

```bash
sudo apt install libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev \
     libxinerama-dev libxcursor-dev libxi-dev libxrandr-dev libxxf86vm-dev \
     libasound2-dev
```

> **Note:** The ALSA dependency is only required for Linux builds. On other platforms the game will build without sound and the engine will continue to run silently.

Run the adventure from the build directory:

```bash
./build/simple3d
```

### Headless validation mode

If you need to compile or test the gameplay logic on a system without OpenGL/GLFW or ALSA libraries, configure CMake with the provided feature flags. This enables a deterministic stub backend that steps the simulation for a few hundred frames, prints a summary, and exits automatically.

```bash
cmake -S . -B build -DSIMPLE3D_ENABLE_OPENGL=OFF -DSIMPLE3D_ENABLE_ALSA=OFF
cmake --build build
./build/simple3d
```

The headless executable still runs the full AI/combat logic while skipping real rendering and audio output, making it useful for CI environments or development containers with limited package access.

## Project Structure

```
├── CMakeLists.txt
├── include
│   ├── audio
│   │   └── AudioEngine.h
│   └── math
│       ├── Mat4.h
│       └── Vec3.h
├── src
│   ├── audio
│   │   └── AudioEngine.cpp
│   └── main.cpp
└── README.md
```

Enjoy carving a path through the crystalline citadel! Feel free to tweak the enemy roster, melodies, or combat parameters to craft your own retro RPG vibe.
