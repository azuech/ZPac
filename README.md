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
- Binary ~45KB (on a ~48KB Z80 RAM limit), external tileset ~49KB streamed to VRAM

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

The output binary is `build/zpac.bin`.

## Running

With the Zeal Native Emulator:

```bash
zeal-native -u build/zpac.bin -H data/
```

The `-u` flag loads the binary directly. The `-H` flag mounts the `data/` directory as drive `H:` — this is where the game looks for `zpac_tileset.bin` at startup. The tileset must be accessible as `H:/zpac_tileset.bin` for the game to run.

With the real hardware:

`zpac.bin`
The file `zpac_tiles.bin` (present in the folder `data` on this repo) must be present on the same path of the executable.

## Project Structure

```
zpac/
├── src/
│   ├── main.c           # Entry point, game state machine
│   ├── game_loop.c/h    # Game loop, level logic
│   ├── ghost.c/h        # Ghost AI, targeting, state machine
│   ├── maze_logic.c/h   # Maze structure, collision map, dots
│   ├── dots.c/h         # Dot eating, deferred tile replacement
│   ├── input.c/h        # Keyboard + SNES controller abstraction
│   ├── sound.c/h        # PSG audio effects
│   ├── fruit.c/h        # Fruit bonus spawn and collision
│   ├── cutscene.c/h     # Intermission cutscenes (3 acts)
│   ├── level256.c/h     # Level 256 split-screen bug simulation
│   ├── zpac_types.h     # Shared structs, enums, speed tables
│   └── zpac_maze_data.h # Palette, tile maps, sprite defines
├── data/
│   └── zpac_tileset.bin # Pre-built tileset (~49KB, required at runtime)
├── Docs/
│   |── Bresenham_acc.md # Description of the Bresenham Accumulator used for the gameflow
│   |── Credits.md       # External sources used for the project
|   └── Project_devlog.md# Project Development log
├── CMakeLists.txt
├── LICENSE              # Apache 2.0
├── CREDITS.md           # References and acknowledgments
└── README.md
```

## Architecture Notes

- **Mode 6** is used for the tile budget: the game needs 384+ unique tiles, exceeding Mode 5's 256-tile limit
- **Tileset streaming**: the ~49KB tileset (in `data/`) is streamed to VRAM at startup via HostFS (`H:/zpac_tileset.bin`), keeping the binary small
- **Bresenham accumulator** drives sub-tile movement to eliminate frame-skip jitter
- **Deferred tile replacement** uses a 4-frame delay to hide dot-eating transitions under the player sprite
- **Ghost AI** follows the classic per-ghost targeting rules with scatter/chase mode transitions

## Development Methodology

ZPac is built using an AI-orchestrated development model where a human architect drives the project and an AI handles the technical implementation:

- The **project architect** defines the development strategy phase by phase, sets design constraints, makes architectural decisions, identifies when an approach isn't working, and validates every result on the emulator. Key decisions — from choosing Mode 6 over Mode 5, to the external tileset strategy, to prioritizing the authentic 32-bit speed accumulator over simplified alternatives — stem from his ability to ask the right questions and recognize when a hardware constraint demands a design rethink.
- The **AI** serves as the technical driver: analyzing hardware documentation, proposing implementation options with trade-offs, writing C/Z80 code, and debugging. Claude prepares self-contained prompts that are executed via **Claude Code** in the Linux development environment.

The workflow cycles between strategic discussion (where to go next, which trade-offs to accept), implementation (Claude Code writing and compiling), and validation (emulator testing with visual and audio feedback). Neither could complete the project alone: the human holds the big picture and makes the calls; the AI handles the complexity of Z80 code and niche hardware APIs.

This approach allows a non-embedded-systems developer to build a complete Z80 game through structured AI collaboration.

## Current Status

**Phase 10 complete** — all planned development phases are finished:

- Phases 1–2: Maze rendering, sprites, font, palette (Mode 6, 329+ tiles, 2×2 composite sprites)
- Phase 3: Interactive gameplay at 60fps (tile+sub movement, dot eating, tunnel)
- Phases 4–6: Complete ghost AI with scatter/chase, frightened mode, eyes, ghost house, level completion
- Phase 7: Full PSG audio (siren, waka-waka, fright siren, ghost eaten, death jingle)
- Phase 8: Polish — lives, death animation, fruit bonus, Cruise Elroy, per-level timing tables
- Phase 9: Title screen, attract mode with demo AI, coin/credit system, high score
- Phase 10: 3-act intermission cutscenes with music, Level 256 split-screen bug, arcade-speed calibration

Binary ~45KB on ~48KB Z80 RAM limit; tileset ~49KB loaded externally to VRAM at startup. Game speed currently within 0.5% of the arcade original, calibrated on the Zeal Native Emulator running on a low-spec x64 Linux PC. A port to the physical Zeal 8-bit Computer is planned in the coming weeks, with timing recalibration to match the real platform.

## Acknowledgments

- [Zeal 8-bit Computer](https://zeal8bit.com) by Zeal8bit — the target platform
- [ZGDK](https://github.com/Zeal8bit/Zeal-Game-Dev-Kit) — Zeal Game Development Kit
- The various publicly available technical references on classic arcade maze game behavior

## Disclaimer

This is a non-commercial, educational hobby project. The original game design, maze layout, and character behaviors are the intellectual property of their respective original authors and rights holders. This project is not affiliated with or endorsed by them. No original ROM code or assets are used — all code is a clean-room reimplementation based on publicly available documentation.

## License

The source code in this repository is licensed under the Apache License 2.0 — see the [LICENSE](LICENSE) file for details. This license applies to the implementation code only, not to the original game design.
