# ZPac

A retro arcade maze game for the [Zeal 8-bit Computer](https://zeal8bit.com), featuring tile-based graphics, ghost AI, and classic gameplay — built entirely in C (SDCC) targeting the Z80 processor and Zeal Video Board.

## About

ZPac is a white-room reimplementation of a classic 1980s arcade maze game, developed for the Zeal 8-bit Computer platform. The project is also an experiment in **AI-orchestrated development**: Claude (Anthropic) serves as the technical driver, while a human project manager directs architecture and validates each step on the emulator.

No original ROM code is used. All behavior is reimplemented from publicly available documentation and behavioral observation.

## Features

- Tile-based maze rendering at 1.5× scale (Mode 6, 640×480, 4bpp)
- 2×2 composite hardware sprites for all characters
- Four ghosts with distinct AI personalities (scatter, chase, frightened modes)
- Cruise Elroy speed tiers
- Scatter/chase timing with per-level tables
- Arcade-accurate PSG audio (death jingle, ghost eat, fruit eat, siren)
- Lives system with HUD, fruit bonus, extra life at 10,000 points
- 12-frame death animation
- External tileset streaming via HostFS (~43KB to VRAM)
- Binary size ~7–8KB, ~40KB free Z80 RAM for gameplay

## Requirements

- [SDCC](https://sdcc.sourceforge.net/) (v4.2.0+) — Small Device C Compiler
- [z88dk](https://github.com/z88dk/z88dk) — z80asm assembler
- [Zeal 8-bit OS](https://github.com/Zeal8bit/Zeal-8-bit-OS) source (for headers and build)
- [Zeal Video Board SDK](https://github.com/Zeal8bit/Zeal-VideoBoard-SDK)
- [Zeal Native Emulator](https://github.com/Zeal8bit/Zeal-Native-Emulator) or Zeal WebEmulator for testing
- CMake

### Environment Variables

```bash
export ZOS_PATH=/path/to/Zeal-8-bit-OS
export ZVB_SDK_PATH=/path/to/Zeal-VideoBoard-SDK
```

## Building

```bash
mkdir build && cd build
cmake ..
make
```

The output binary is `build/zpac_test.bin`.

## Running

With the Zeal Native Emulator:

```bash
zeal-native -u build/zpac_test.bin -H /path/to/zpac_assets/
```

The `-u` flag loads the binary directly, `-H` mounts the HostFS directory containing `zpac_tileset.bin`.

## Project Structure

```
zpac/
├── src/
│   ├── main.c           # Entry point, game state machine
│   ├── game.c/h         # Game loop, level logic
│   ├── pacman.c/h       # Player movement, animation
│   ├── ghost.c/h        # Ghost AI, targeting, state machine
│   ├── maze.c/h         # Maze structure, collision, dots
│   ├── input.c/h        # Keyboard + SNES controller abstraction
│   ├── render.c/h       # Tilemap, sprite rendering
│   ├── sound.c/h        # PSG audio effects
│   ├── hud.c/h          # Score, lives, fruit display
│   ├── fruit.c/h        # Fruit bonus spawn and collision
│   └── data.h           # Constants, lookup tables
├── assets/
│   ├── zpac_maze_data.h # Palette + tileset data + maps
│   └── zpac_tileset.bin # External tileset (43KB, loaded at runtime)
├── CMakeLists.txt
├── LICENSE              # Apache 2.0
└── README.md
```

## Architecture Notes

- **Mode 6** is used for the tile budget: the game needs 329+ unique tiles, exceeding Mode 5's 256-tile limit
- **Tileset streaming**: the 43KB tileset is stored externally and streamed to VRAM at startup via HostFS (`H:/zpac_tileset.bin`), keeping the binary small
- **Bresenham accumulator** drives sub-tile movement to eliminate frame-skip jitter
- **Deferred tile replacement** uses a 4-frame delay to hide dot-eating transitions under the player sprite
- **Ghost AI** follows the classic per-ghost targeting rules with scatter/chase mode transitions

## Development Methodology

This project uses an AI-orchestrated development workflow:

1. **Claude** (Anthropic) prepares detailed implementation prompts
2. **Claude Code** executes the implementation
3. The **project manager** validates each step on the Zeal emulator
4. Results feed back into the next iteration

This approach allows a non-embedded-systems developer to build a complete Z80 game through structured AI collaboration.

## Current Status

**Phase 8 complete** — core gameplay loop with ghost AI, audio, lives, fruit, Cruise Elroy, and scatter/chase timing are all implemented. Phase 9 (title screen and attract mode) is next.

## Acknowledgments

- [Zeal 8-bit Computer](https://zeal8bit.com) by Zeal8bit — the target platform
- [ZGDK](https://github.com/Zeal8bit/Zeal-Game-Dev-Kit) — Zeal Game Development Kit
- The various publicly available technical references on classic arcade maze game behavior

## Disclaimer

This is a non-commercial, educational hobby project. The original game design, maze layout, and character behaviors are the intellectual property of their respective original authors and rights holders. This project is not affiliated with or endorsed by them. No original ROM code or assets are used — all code is a clean-room reimplementation based on publicly available documentation.

## License

The source code in this repository is licensed under the Apache License 2.0 — see the [LICENSE](LICENSE) file for details. This license applies to the implementation code only, not to the original game design.
