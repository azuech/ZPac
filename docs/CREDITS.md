# ZPac — Credits & Acknowledgments

This document acknowledges the third-party sources, tools, platforms, and
reference materials that made the ZPac project possible.

ZPac is a white-room reimplementation: all source code is original, written
from scratch in C89 for the Zeal 8-bit Computer. No code has been copied
from any external project. The references listed below were studied for
behavioral accuracy and technical understanding.

---

## Behavioral References

### Andre Weissflog (floooh)

Available on GitHub under the floooh account.  
License: MIT

A faithful single-file C99 clone of the original arcade game, used as
our primary behavioral reference for ghost AI, scatter/chase timing,
dot counter logic, ghost house exit rules, and pathfinding mechanics.
ZPac's gameplay algorithms were informed by studying this implementation
but rewritten entirely for the Z80 target with different data structures,
movement system, and platform-specific optimizations.

### Jamey Pittman — "The Dossier"

The definitive community guide to the original arcade game's internal
mechanics. Used as the authoritative source for ghost targeting strategies,
speed tables, level progression, scatter/chase phase timing, and special
behaviors such as Cruise Elroy and the red zone restrictions. A PDF copy
was included in the floooh repository referenced above.

### Chris Lomont — "Arcade Hardware Emulation Guide" (v0.1, October 2008)

Available at: <https://www.lomont.org>

A technical reference for the original arcade hardware: video memory
layout, sound register architecture, sprite handling, tile ROM structure,
and waveform sound generator (WSG) operation. Used to understand the
hardware-level behavior that informs correct emulation of audio and
video timing.

### Don Hodges — "Splitting Apart the Split Screen"

Available at: <https://donhodges.com>

Detailed analysis of the level 256 split-screen bug caused by the
fruit-drawing routine overflow. Used as the primary reference for
ZPac's level 256 corruption simulation, including the exact tile and
color values written to video memory by the original Z80 code.


### MAME (Multi-Arcade Machine Emulator)

Used as the benchmark reference standard for visual and behavioral
accuracy. All timing calibrations, animation speeds, and gameplay
behaviors in ZPac were validated against MAME running the original
arcade ROM set.

---

## Target Platform

### Zeal 8-bit Computer — Designed by Zeal 8-bit

Website: <https://zeal8bit.com>  
GitHub: <https://github.com/Zeal8bit>

The hardware platform ZPac runs on: a Z80-based homebrew computer
with FPGA-based Video Board, designed for educational and community
use.

### Zeal 8-bit OS

Repository: <https://github.com/Zeal8bit/Zeal-8-bit-OS>  
License: Apache 2.0

The operating system providing the kernel, filesystem (including
HostFS for emulator-based development), keyboard driver, and syscall
interface that ZPac builds upon.

### Zeal Video Board SDK (ZVB SDK)

Repository: <https://github.com/Zeal8bit/Zeal-VideoBoard-SDK>  
License: Apache 2.0 (library/tools), CC0-1.0 (examples)

The graphics and audio SDK providing the C API for tilemap rendering,
sprite management, palette control, and PSG sound output. ZPac uses
the ZVB SDK directly for all hardware interaction.

### Zeal Game Development Kit (ZGDK) — by zoul0813

Repository: <https://github.com/zoul0813/zeal-gdklib>

A higher-level game framework for the Zeal platform. Studied during
early project phases for input handling patterns, sprite management
approaches, and sound sequencer architecture. ZPac ultimately uses
the lower-level ZVB SDK directly.

### Z Fighter — by zoul0813

Repository: <https://github.com/zoul0813/zeal-zfighter>

A scrolling shooter game for the Zeal platform, built with ZGDK. Used
as the primary architectural reference for ZPac's multi-file source
structure, game state machine design, and the pattern of separating
game logic into distinct modules (player, enemies, input, rendering,
sound). The file organization of ZPac was directly modeled after
Z Fighter's approach.

### ZGDK Game Examples — by zoul0813

Repositories: Microbe, Bricked, World Engine, Enigma (all available
under the zoul0813 GitHub account).

Additional ZGDK-based games studied during early project phases for
patterns in tile-based rendering, collision handling, and game loop
structure on the Zeal platform.

### Snake — ZVB SDK Example

Part of the Zeal Video Board SDK (CC0-1.0 license). Studied as the
canonical reference for ZVB SDK initialization sequences, tileset
loading, palette setup, keyboard input in raw non-blocking mode,
v-blank synchronization, and SNES controller support. The controller
adapter code (controller.c/h) included in the ZPac project derives
from this example.

### Zeal Native Emulator

The desktop emulator used as the primary testing and development
environment throughout the project. All ZPac builds are validated
here before any hardware testing.

---

## Build Toolchain

### SDCC (Small Device C Compiler)

Website: <https://sdcc.sourceforge.net>

The C compiler targeting the Z80 architecture. ZPac is written in
strict C89 and compiled with SDCC for the Z80 processor.

### z88dk / z80asm

Repository: <https://github.com/z88dk/z88dk>

The Z80 assembler used for linking and assembly-level components
in the build pipeline.

### CMake

Used as the build system, following the patterns established by
ZealOS and the ZVB SDK.

---

## Asset Pipeline

### Python / Pillow

Used in custom scripts for decoding, scaling, slicing, and encoding
graphic assets from their source format into the ZVB-compatible 4bpp
tileset binary and palette data.

---

## Development Methodology

ZPac was developed using an AI-orchestrated workflow, with
[Anthropic's Claude](https://www.anthropic.com) serving as the
technical driver and the human developer (Alessandro) acting as
project manager, navigator, and decision-maker. Claude Code was
used for code execution, and all architectural and creative decisions
were made by the human team lead.

---

*This document was last updated in March 2026.*
