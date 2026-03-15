# ZPac — Development Log

> **ZPac — a classic arcade game clone for Zeal 8-bit Computer with Video Board**
> Version: 2.4 — March 10, 2026
> Author: azuech (PM/Architect) — AI assistance: Claude (Technical Driver)
> Methodology: AI-Orchestrated Development

---

## 1. Project Overview

### 1.1 What Is the Zeal 8-bit Computer

Zeal 8-bit Computer is an open-source project born in 2021 from its creator's passion for the Zilog Z80 — a processor still in production more than 30 years after its launch. The idea is to build a complete homebrew 8-bit computer that retains the simplicity of classic microcomputers like the Commodore 64 or the ZX Spectrum, while adding modern features: VGA output via a dedicated FPGA video board, SD card support, NOR flash storage, and a custom operating system (ZealOS) written entirely in assembly.

Zeal targets an enthusiast niche: makers, retrocomputing hobbyists, and anyone who wants to understand how a computer works "down to the metal". It is available as a solder-it-yourself kit or pre-assembled, sold on Tindie, with an active Discord community of over 100 people. The entire stack — hardware, OS, bootloader, SDK, emulator — is open source under the Apache 2.0 license.

The project also has a strong educational mission: the Z80's simplicity makes it an ideal processor for learning low-level programming, and the FPGA video board — with 128 hardware sprites, a dual-layer tilemap, and a 4-voice PSG — provides a rich playground for retro game development.

### 1.2 Why This Game

The choice of game is not arbitrary. The classic arcade game is probably the best reference target for validating the gaming capabilities of an architecture like the Zeal, for several reasons:

**It is tile-based.** The entire gameplay is built on a tile grid: the maze is a tilemap, the dots are tiles, and the actors' movement follows the grid. This maps perfectly to the Zeal video board's tilemap + sprite architecture.

**It is extraordinarily well-documented.** Between the Pac-Man Dossier (Jamey Pittman), emulation guides (Chris Lomont), and reference implementations like floooh's pacman.c, there is probably no other arcade game analyzed in more depth. Every animation frame, every ghost AI algorithm, every speed table is documented.

**It is an authenticity benchmark.** The arcade version has been ported to dozens of platforms, from the Atari 2600 to modern browsers. The quality of a port is measured by its fidelity to the original behaviors — the ghost chase/scatter pattern, cornering, eating pauses, tunnel slowdown. If ZPac can reproduce these details, it proves that Zeal is a serious gaming platform.

**It has the right complexity.** Simple enough to be built by a two-person team (human + AI), complex enough to stress every subsystem: tilemap, sprites, input, audio, timing, AI.

### 1.3 The Approach: White-Room Study and Reimplementation

A fundamental point: the project studies the original arcade documentation and reference implementations in order to *understand* the game's authentic behavior — how the speed accumulator works, how ghosts make decisions at intersections, exactly when scatter/chase transitions fire. No 1:1 copying is performed.

The implementation is a **white-room rewrite**: each system is redesigned for the Zeal architecture, accounting for its constraints and peculiarities (Mode 6 instead of the original custom video hardware, PSG instead of WSG, 128 sprites instead of 8, a dual-layer tilemap with attributes instead of a separate color RAM). Studying the original documentation clarifies *what* should happen; *how* it happens is designed specifically for the target platform.

### 1.4 The AI-Orchestrated Development Model

ZPac is also an experiment in human-AI collaboration on a non-trivial problem. Writing a game for the Zeal is not like rewriting a classic arcade game in JavaScript in a modern browser: the platform has real memory constraints (48 KB visible), a compiler with limitations (SDCC for Z80), hardware with registers to program at a low level, and an emulator as the only validation tool.

**Azuech** is the architect and project decision-maker. His role goes well beyond "copy-pasting" code: he defines the development strategy phase by phase, sets priorities, validates results on the emulator, identifies when an approach is not working, and decides to change course. Many of the most important architectural decisions — from choosing Mode 6 to the external tileset strategy — stem from his ability to ask the right questions and recognize when a hardware constraint requires a design rethink. He also ensures the project's consistency over time, maintaining the big picture when technical sessions dive into details.

**Claude** is the technical driver: it analyzes hardware documentation, proposes architectures, writes C/assembly code, and debugs problems. It operates within the constraints defined by the architect and explicitly flags when a technical choice has alternatives.

The "magic" lies in the interaction: a PM who understands product constraints and can guide development without necessarily mastering Z80 assembly, combined with an AI that can read datasheets, analyze disassemblies, and produce code for niche platforms, but needs human guidance for strategic decisions. Neither could complete the project alone.

### 1.5 Target Hardware vs. Original Hardware

| Feature | Original Arcade (1980) | Zeal 8-bit Computer |
|---|---|---|
| CPU | Z80A @ 3.072 MHz | Z80 @ 10 MHz |
| RAM | ~2 KB | 512 KB (banked, 48 KB visible) |
| Video | Custom hardware, 224×288, 8 sprites | FPGA Video Board, 640×480, 128 sprites |
| Tiles | 256 tiles 8×8, 2bpp, 4 colors | 512 tiles 16×16, 4bpp, 16 palette groups |
| Audio | Custom WSG 3 voices, 4-bit wavetable | PSG 4 voices + 1 sample, square/tri/saw/noise |
| Input | 4-direction joystick | PS/2 keyboard + SNES controller adapter |
| Storage | 16 KB ROM | SD card / HostFS |
| OS | None (bare metal) | ZealOS |

The key differences that shape every project decision: the higher resolution forces us to scale graphics 1.5×; 512 tiles let us fit all assets in a single tileset; 4 PSG voices compensate for the missing original wavetables; banked memory requires careful layout but provides sufficient space for full gameplay. The SNES adapter connected to the Z80 PIO Port A provides authentic physical input with a directional D-pad, perfect for this game — alongside the PS/2 keyboard for emulator development.

---

## 2. Tech Stack and Development Environment

### 2.1 Toolchain

| Component | Version | Role |
|---|---|---|
| SDCC | 4.2.0+ | C compiler for Z80 |
| z88dk-z80asm | — | Assembler for ASM modules |
| CMake | 3.20+ | Build system |
| ZealOS | latest | Target operating system |
| ZVB SDK | latest | Low-level graphics and audio API |
| ZGDK | latest | Game development framework (sprites, input, tilemap) |

### 2.2 Development Environment

The project is developed on Ubuntu 24.04 LTS. The Zeal Native emulator (`zeal-native`) is the only validation tool — every step must produce verifiable output.

Critical environment variables:

```bash
export ZOS_PATH=~/Zeal-8-bit-OS
export ZVB_SDK_PATH=~/Zeal-VideoBoard-SDK
```

### 2.3 Emulator Commands

```bash
# Standard run (binary + external tileset)
zeal-native -u build/zpac.bin -H build/

# -u  = load user program
# -H  = mount host directory as H: drive (hostfs)
# The tileset is loaded at runtime from H:/zpac_tiles.bin
```

### 2.4 Development Workflow

The workflow reflects the division of roles:

1. **Azuech analyzes the project state**, identifies the next objective, and defines the design constraints — for example: "use the authentic 32-bit speed system, not simplified tick-skipping" or "the tileset must live in an external file, inline compression is not enough"
2. **Azuech and Claude** discuss the technical approach in the Claude project. Claude proposes options, Azuech chooses direction and validates feasibility
3. **Claude** generates a self-contained prompt with all technical context inline (code snippets, hardware constants, specs, reference patterns) — prompts must be self-contained because Claude Code has no access to the project knowledge base
4. **Azuech** takes the prompt into a **Claude Code** session that has direct access to the Linux development environment; Claude Code implements, compiles, and resolves errors
5. **Azuech validates on the emulator**, diagnoses any visual or behavioral issues, and decides whether the result is acceptable or needs iteration

This cycle has proven effective because it separates concerns: architectural decisions and validation remain with the human who holds the big picture; technical implementation is delegated to the AI, which can handle the complexity of Z80 code and hardware APIs.

---

## 3. Fundamental Architectural Decision: Video Mode

This was the project's first critical decision and shaped everything that followed. It is also a good example of the workflow: Azuech asked the question — "are we sure 320×240 is enough for all our assets?" — which led to discovering the 256-tile constraint and the subsequent migration to Mode 6, before any significant code had been written.

### 3.1 The Problem: Tile Budget

The original arcade version uses 256 8×8 2bpp tiles. The ZPac graphics pipeline — which scales everything 1.5× — requires **329 unique tiles** at 16×16 pixels: 177 for the maze, 20 for ZPac's movement, 48 for the death animation, 32 for ghosts, 8 for frightened, 32 for fruit, 42 for the font, 1 blank.

The Zeal Video Board offers two relevant graphics modes:

**Mode 5 (8bpp, 320×240):** max 256 tiles, 256 bytes per tile, global palette of 256 colors, layer1 used as transparent foreground. *Insufficient: 329 > 256 tiles.*

**Mode 6 (4bpp, 640×480):** max 512 tiles, 128 bytes per tile, 16 palette groups of 16 colors each, layer1 used as an attribute map (palette, flip X/Y, tile index bit 8), hardware tile flipping.

### 3.2 The Choice: Mode 6

Mode 6 solves three problems at once:

1. **Tile budget:** 512 slots available, ZPac uses 329 (341 with score sprites), leaving 171 free for future additions
2. **Memory per tile:** 128 bytes/tile instead of 256 — the full tileset occupies ~43 KB in VRAM instead of ~84 KB
3. **Palette groups:** 16 independent groups of 16 colors allow recoloring the same tiles for different characters (ghost eyes, for example, reuse the same tile data as normal ghosts with a different palette — zero extra tiles)

Hardware flip (X and Y) on tiles is a significant bonus: ZPac's animation uses the same frames for left/right and up/down, differentiated only by the flip flag.

### 3.3 Impact on Screen Layout

The 640×480 resolution with 16×16 tiles produces a 40×30 tile grid — much larger than the original 28×36 tiles at 8px. The original arcade maze (28×31 tiles at 8px) is scaled 1.5× and occupies 21×24 Zeal tiles at 16px, positioned at offset (9, 3) to center it on screen with HUD space above (3 rows) and below (3 rows).

The conversion between arcade logical coordinates and screen pixels is:

```
screen_pixel = original_logical_tile × 12 + maze_offset
```

The factor 12 (= 8 original pixels × 1.5) is the fundamental scale ratio that runs through all the code.

---

## 4. Graphics Pipeline

### 4.1 Asset Source and White-Room Approach

For the graphics pipeline, the asset geometry and color data were reconstructed by studying publicly available arcade documentation and reference implementations. The assets were not copied: they were redrawn from specifications to match the original shapes, colors, and proportions, then converted to the Zeal video board's specific format via a transformation pipeline designed from scratch for Mode 6.

| Source Data | Content | Size |
|---|---|---|
| Tile geometry | 256 tiles 8×8, reconstructed at 1.5× | ~4 KB source |
| Sprite geometry | 64 sprites 16×16, reconstructed at 1.5× | ~4 KB source |
| Hardware palette | RGB values from resistor network documentation | 32 bytes |
| Palette indirection | Color code → hardware color mapping | 256 bytes |

### 4.2 Conversion Pipeline

The path from ROM data to Zeal tiles goes through several stages:

**Maze tiles:**
Source tile 8×8 (2bpp) → bitplane decode → pixel composition via ASCII maze map →
full image 224×248 → scale 1.5× (to 336×372) → slice into 16×16 tiles (21×24 grid) →
deduplication → 4bpp encoding (128 bytes/tile)

**Character sprites:**
Source sprite 16×16 (2bpp, 8 strips) → decode → scale 1.5× (to 24×24) → center in
32×32 canvas (4px padding) → slice into 4 quadrants 16×16 → quadrant deduplication →
4bpp encoding

**Font:**
Source tile 8×8 (alphanumeric subset) → decode → scale 2× → 16×16 tile →
4bpp encoding

**Palette:**
Documented hardware colors (resistor network) → RGB888 → grouping into 16 palette groups of 4 colors
(expanded to 16 slots) → RGB565 encoding (512 bytes total)

### 4.3 Deduplication and Optimizations

Sprite quadrant deduplication was very effective: the initial estimate of 438 tiles dropped to 329 actual tiles (341 with score sprites). Many ghost quadrants are identical across animation frames, and ghost eyes reuse the tile from frame 0 of each direction with a different palette.

### 4.4 2×2 Composite Sprites

Each character is a composite of 4 hardware sprites arranged in a 2×2 grid:

```
[TL][TR]   ← 16×16 each
[BL][BR]

Total canvas: 32×32 pixels
Actual content: 24×24 (1.5× of original 16×16)
Padding: 4px per side to center
```

This approach uses 4 of the 128 hardware sprites per character (20 total for 5 characters), leaving 108 free for fruit and effects.

### 4.5 Palette Groups Implemented

| Group | Use | Main Colors |
|---|---|---|
| 0 | Maze (walls + dots) | Blue, peach, white, pink |
| 1 | ZPac (player) | Yellow, orange |
| 2 | Blinky (red) | Red, white, blue |
| 3 | Pinky (pink) | Pink, white, blue |
| 4 | Inky (cyan) | Cyan, white, blue |
| 5 | Clyde (orange) | Orange, white, blue |
| 6 | Frightened (blue) | Dark blue, white, peach |
| 7 | Frightened flash (white) | White, red |
| 8 | Ghost eyes | White, blue |
| 9 | Font (white) | White |
| 10-15 | Fruit / reserved | Various |

The palette group system is one of Mode 6's most exploited features: same tile data, different visual appearance. Ghosts share the same animation tiles, differentiated only by their palette group.

---

## 5. Memory Architecture

### 5.1 The Constraint and the Real Challenges

The Z80 can address 64 KB, of which ~48 KB are available for the user program on ZealOS. At the end of Phase 2, the binary had reached ~49 KB with the inline tileset (42 KB of tile data + ~7 KB of code) — no space remained for gameplay.

**First step: external tileset via HostFS.** Moving the tileset to a separate binary file (`zpac_tileset.bin`, 43,648 bytes), loaded at runtime via the emulator's host filesystem, immediately freed ~40 KB of working RAM. The tileset loads in streaming — 512-byte chunks written directly to VRAM — and after loading resides entirely in VRAM with zero footprint in Z80 RAM.

```
BEFORE:  Binary ~49 KB (inline tileset 42KB + code 7KB) → ~285 bytes margin
AFTER:   Binary ~7-8 KB (palette 512B + maps 1KB + code ~6KB) → ~40 KB margin
```

That headroom, however, was progressively consumed as gameplay systems grew. The memory constraint remained critical through the entire project and required sustained attention on several fronts:

**Resolution scaling (1.5×).** The coordinate system mismatch between the original 8px tile grid and the Zeal 16px tile grid runs through every subsystem — movement, collision, sprite positioning, deferred tile replacement. Getting this right without divisions (prohibited on Z80 at ~300+ cycles each) required the tile+sub fixed-point system described in §8, and careful alignment validation at every phase.

**Audio data compression.** The intermission music, once converted to a tick-by-tick PSG array, occupied 1,316 bytes in its raw form. RLE compression brought it to 177 bytes — essential at the point where the binary was within 1 KB of the limit. The same discipline applied to all audio tables.

**Binary discipline.** Late in the project the binary reached 47,014 bytes against a ~48 KB ceiling. Every new feature required a size impact assessment. One subtle trap: `zpac_palette` declared `static const` in a header adds 512 bytes to every translation unit that includes it — a non-obvious cost that had to be identified and managed explicitly.

**Attract mode AI.** Implementing a convincing autoplay demo that navigates the maze, reacts to ghosts, and triggers the energizer sequence without human input required several calibration iterations. A ghost danger radius of 4 Manhattan tiles paralyzed ZPac at intersections; the correct value turned out to be 1 tile (immediate adjacency only). Distance-squared navigation outperformed Manhattan distance for smooth corridor following.

**Animation synchronization.** Keeping sprite animations, sound events, scatter/chase timers, frightened flash, and cutscene choreography in sync at a stable frame rate — across a platform where even a division instruction costs 300+ cycles — required careful frame-budget accounting and the `fast_wait_vblank()` optimization that replaced the SDK call. Final calibration targets 75fps on real hardware.

### 5.2 Key Technical Discoveries

**L11:** `gfx_tileset_load()` with `from_byte` and `TILESET_COMP_NONE` works for chunk streaming — the tileset does not need to be in RAM all at once.

**L12:** The emulator's `-H` flag mounts a host directory as drive `H:`, providing a straightforward path for external asset loading during development.

### 5.3 Current File Structure

```
zpac_phase1_test/
├── src/
│   ├── main.c              (~8 KB)  Entry point, tileset loader, render, game init
│   ├── zpac_maze_data.h    (~330 lines)  Palette + maps + defines (NO inline tileset)
│   ├── zpac_types.h        Structs pacman_t, direction enum, cell flags
│   ├── maze_logic.h        maze_logic declarations
│   └── maze_logic.c        Array maze_logic[31][28] with bit flags
├── zpac_tiles.bin           (49,152 bytes)  Full raw 4bpp tileset (384 tiles)
├── CMakeLists.txt           CMake build config
└── build/
    └── zpac.bin             Compiled binary (~46 KB)
```

---

## 6. Initialization Sequence (Validated on Emulator)

The order of steps is critical — any deviation produces artifacts or crashes:

```c
// 1. Non-blocking keyboard (raw mode for game input)
ioctl(DEV_STDIN, KB_CMD_SET_MODE, (void*)(KB_READ_NON_BLOCK | KB_MODE_RAW));

// 2. Screen OFF during setup
gfx_enable_screen(0);

// 3. Initialize Mode 6
gfx_initialize(ZVB_CTRL_VID_MODE_GFX_640_4BIT, &ctx);

// 4. Blank tile + clear BOTH layers (CRITICAL in Mode 6)
gfx_tileset_add_color_tile(&ctx, TILE_BLANK, 0);
clean_tilemap();  // layer0 AND layer1

// 5. Palette (512 bytes RGB565, 16 groups × 16 colors × 2 bytes)
gfx_palette_load(&ctx, zpac_palette, 512, 0);

// 6. Tileset from external file (streaming to VRAM in 512B chunks)
load_tileset_from_file();  // open("zpac_tiles.bin") → read → gfx_tileset_load

// 7. Recreate blank tile (overwritten by tileset load in step 6)
gfx_tileset_add_color_tile(&ctx, TILE_BLANK, 0);

// 8. Draw maze + HUD
render_maze();   // layer0 (tile index) + layer1 (palette group 0 attributes)
render_hud();    // font tiles with palette group 9

// 9. Position sprites (2×2 composite)
set_sprite_2x2(HW_SPR_PACMAN, tiles_pacman, PAL_PACMAN, x, y);
// ... repeat for 4 ghosts

// 10. Screen ON
gfx_enable_screen(1);
```

**Critical lesson (L4):** in Mode 6, layer1 is the attribute map, not a transparent foreground. Every tile requires writes to both layer0 (index) AND layer1 (palette group + flip flags). Forgetting to clear layer1 at step 4 produces random colors.

---

## 7. Maze Data Structures

### 7.1 Dual-Grid: Logic vs. Visual

The project maintains two parallel maze representations:

**Logic grid (28×31):** uses the original arcade maze coordinates, with 8-pixel tiles scaled to 12 effective pixels. Each cell is a byte with bit flags encoding gameplay properties. All game decisions (collisions, AI, intersections) operate on this grid.

**Visual grid (21×24):** uses the Zeal 16×16-pixel tile coordinates. Used only for tilemap rendering. The mapping between the two grids is handled by the 1.5× scale factor.

### 7.2 Maze Logic Map

Array `maze_logic[31][28]` (868 bytes) where each byte encodes:

| Bit | Flag | Meaning |
|---|---|---|
| 0 | WALKABLE | Cell is passable (corridor, not wall) |
| 1 | DOT | Contains a dot (10 points) |
| 2 | ENERGIZER | Contains an energizer/power pellet (50 points) |
| 3 | INTERSECTION | Intersection — ghosts make decisions here |
| 4 | NO_UP | No-up zone for ghosts (2 cells above the ghost house) |
| 5 | TUNNEL | Tunnel zone (ghosts slow to ~40%) |
| 6 | GHOST_HOUSE | Door and interior area of the ghost house |
| 7 | — | Reserved |

The maze contains exactly 244 collectibles: 240 dots + 4 energizers, matching the original.

### 7.3 Tunnel

The tunnel (the pair of horizontal passages on the sides of the maze) is a special case: when an actor's X position exceeds the border, it reappears on the opposite side. Ghosts are subject to a significant slowdown (~40% of base speed) in the tunnel — an authentic arcade behavior that is important to reproduce because it influences game strategy.

---

## 8. Movement System

The movement system is a clear example of the white-room approach: the original behavioral documentation is studied to understand *what* the speed accumulator does and *why* the 32-bit patterns produce that precise game feel, then the mechanism is reimplemented for the Zeal architecture (different clock, different frame timing, 1.5× scaled coordinates).

### 8.1 Speed Accumulator — Implementation Evolution

**First iteration (Phase 3.2):** the original system used pixel coordinates with a 32-bit rotating accumulator. It worked but performance on the emulator was too slow for smooth gameplay.

**Phase 3 refactor (post-optimization):** critical discovery — the main bottleneck was division/modulo operations to calculate `tile_x` and `tile_y` from pixel coordinates. On Z80, a 16-bit division costs ~300+ cycles. The solution was a complete restructure toward a **tile+sub** system:

- `tile_x`, `tile_y`: position in the logical grid (0–27, 0–30)
- `sub_x`, `sub_y`: sub-tile position as 16-bit fixed-point (256 = 1 full tile)
- Zero divisions: all alignment checks use direct comparisons on `sub`

The speed system moved from an 8-bit Bresenham accumulator to a 16-bit fixed-point accumulator, more robust for handling fractional speeds.

```c
// Tile+sub system: no divisions
// sub ranges from 0 to 255, 128 = tile center
if (sub_x == 128 && sub_y == 128) {
    // At center → can decide new direction
}
```

**Another critical optimization:** replacement of `gfx_wait_vblank()` (SDK) with a custom `fast_wait_vblank()` that reads the I/O register directly — the SDK performed costly MMU operations on every call.

Result: jump from ~21fps to smooth emulator performance, later recalibrated for 75Hz hardware (see §17.3).

### 8.2 32-bit Speed Accumulator

The original arcade uses an elegant speed system based on a 32-bit accumulator. Each frame, a 32-bit pattern is rotated left (shift-left with carry). If the outgoing bit is 1, the actor moves 1 pixel; if 0, it stays still.

Example: the pattern `0xD5555555` for ZPac at level 1 has ~81% of bits set to 1, producing movement in ~81% of frames — equivalent to ~80% of maximum speed.

```
Pattern:  1101 0101 0101 0101 0101 0101 0101 0101
Frame:    ^move ^stay ^move...
```

We chose this authentic system over the simplified tick-skipping in floooh/pacman.c — a strategic decision by Azuech that prioritized arcade fidelity.

### 8.3 Tile-Center Decisions

Both ZPac and ghosts can only change direction when aligned to the center of a logical tile. In the tile+sub system:

```c
if (actor.sub_x == 128 && actor.sub_y == 128) {
    // At center → can decide new direction
}
```

This matches the tile-center decision behavior documented in arcade technical references.

### 8.4 Cornering

ZPac has an advantage over ghosts: it can start turning up to ~4 pixels before reaching the intersection center ("pre-turn"). Ghosts turn only at the exact center. This asymmetry is intentional in the original and contributes to the feeling that ZPac is more agile than its pursuers.

### 8.5 Sprite Coordinates

The conversion from arcade logical coordinates to Zeal screen sprite positions:

```c
#define MAZE_PX_X  (9 * 16)   // 144 — horizontal maze offset
#define MAZE_PX_Y  (3 * 16)   // 48  — vertical maze offset

// Convert original coordinates (8px grid) → screen pixels
// The -4 compensates for the 32×32 canvas padding (24×24 content centered)
#define ORIG_TO_SPR_X(col) (MAZE_PX_X + (col) * 12 - 4)
#define ORIG_TO_SPR_Y(row) (MAZE_PX_Y + (row) * 12 - 4)
```

---

## 9. Animations

### 9.1 ZPac

4 cyclic frames: wide open → medium → closed → medium. The phase is computed from an animation counter incremented on every pixel of movement:

```c
// phase = (anim_tick / 2) & 3
// Horizontal: tiles {44, 46, 48, 46}  — left = flipX
// Vertical:   tiles {45, 47, 48, 47}  — up = flipY
```

Tile 48 (closed mouth) is shared across all directions.

### 9.2 Ghosts

2 frames per direction, alternated via a slower timer:

```c
// phase = (anim_tick / 8) & 1
// tiles[4dir][2frame] = {{32,33},{34,35},{36,37},{38,39}}
// Color determined by palette group, not by tile data
```

### 9.3 Hardware Sprite System

| HW Sprite | Character |
|---|---|
| 0–3 | ZPac (2×2 composite) |
| 4–7 | Blinky (red) |
| 8–11 | Pinky (pink) |
| 12–15 | Inky (cyan) |
| 16–19 | Clyde (orange) |
| 20–23 | Fruit (when active) |
| 24–127 | Free |

---

## 10. Dot Eating and Deferred Tile Replacement

### 10.1 Basic Mechanic

When ZPac is centered on a tile (sub_x == 128, sub_y == 128) with the CELL_DOT or CELL_ENERGIZER flag set, consumption triggers:

- DOT/ENERGIZER bit cleared in logic map
- Score incremented (+10 or +50)
- Eating pause: 1 frame for dot, 3 frames for energizer
- `dots_remaining` decremented

### 10.2 Deferred Tile Replacement

The problem: the logical grid (28×31 at 12px) and the visual grid (21×24 at 16px) are not aligned 1:1 due to the 1.5× scale factor. A single visual tile can contain portions of 2–4 logical cells.

The solution: **4-frame deferred tile replacement**. When a dot is eaten, the visual tile is not replaced immediately — it is queued with a 4-frame timer. ZPac's sprite covers the visual transition during this interval.

At the time of the actual replacement, an on-demand check is performed: if any logical cell still active (with an uneaten dot) maps to the same visual tile, the replacement is cancelled. This resolves the issue of dots at perpendicular crossroads.

### 10.3 No-Dot Tile Variants

"Empty corridor" tiles (without dot) are generated at runtime during tileset loading, not duplicated in the .bin file. Savings: dozens of tiles in the budget.

---

## 11. Ghost AI

### 11.1 State Machine

```
HOUSE → EXITING → SCATTER ↔ CHASE
                      ↓         ↓
                 FRIGHTENED → EYES → ENTERING → HOUSE
```

Each ghost has a current state and transitions between states in response to global events (scatter/chase timer, energizer eaten) or personal events (dot counter reached, collision).

### 11.2 ghost_t Data Structure

```c
typedef struct {
    uint8_t  tile_x, tile_y;      // logical grid position
    uint16_t sub_x, sub_y;        // sub-tile position (fixed-point)
    uint8_t  dir_current;         // current direction
    uint8_t  dir_next;            // pre-computed direction (look-ahead)
    uint32_t speed_acc;           // 32-bit speed accumulator
    uint8_t  state;               // HOUSE/EXITING/SCATTER/CHASE/FRIGHTENED/EYES/ENTERING
    uint8_t  target_x, target_y;  // current target tile
    uint8_t  anim_frame;
    uint8_t  anim_counter;
    uint8_t  reversal_pending;    // deferred reversal flag
} ghost_t;
```

### 11.3 Scatter Corners

| Ghost | Corner | Target Tile |
|---|---|---|
| Blinky (red) | Top right | (25, 0) |
| Pinky (pink) | Top left | (2, 0) |
| Inky (cyan) | Bottom right | (27, 30) |
| Clyde (orange) | Bottom left | (0, 30) |

### 11.4 Chase Targeting

```
Blinky:  target = ZPac's tile (direct chase)
Pinky:   target = 4 tiles ahead of ZPac [arcade BUG: if UP → +4up +4left]
Inky:    target = vector Blinky→(2 tiles ahead of PM) × 2 [same BUG]
Clyde:   if dist² > 64 tiles → like Blinky; otherwise → own scatter corner
```

### 11.5 Look-Ahead and Pathfinding

The ghost decides its direction **one tile before** the intersection. Algorithm:

1. For each possible direction from the look-ahead tile (excluding: reversal, walls, GHOST_HOUSE)
2. If the tile is in a no-up zone → exclude DIR_UP
3. Choose the direction with minimum distance² from the target
4. Tie-breaking: UP > LEFT > DOWN > RIGHT (arcade priority order)

### 11.6 Scatter/Chase Timer (Level 1)

| Phase | Duration | Frames @75fps |
|---|---|---|
| Scatter 1 | 7s | 525 |
| Chase 1 | 20s | 1500 |
| Scatter 2 | 7s | 525 |
| Chase 2 | 20s | 1500 |
| Scatter 3 | 5s | 375 |
| Chase 3 | 20s | 1500 |
| Scatter 4 | 5s | 375 |
| Chase 4 | Permanent | — |

On phase change: `reversal_pending` set on all active ghosts.

### 11.7 Ghost House Exit

- **Pinky**: exits immediately (threshold 0 dots)
- **Inky**: exits after ~30 dots eaten
- **Clyde**: exits after ~60 dots eaten
- **Anti-stall timer**: if ZPac doesn't eat for ~300 frames (~4s at 75fps), the next ghost is force-released
- **Bouncing in house**: vertical bounce in the room's center tile
- **Exiting path**: ghost house center → door → exit tile → normal maze

### 11.8 Speed Differentiation

| State | Relative Speed |
|---|---|
| Chase/Scatter | 75% |
| Frightened | 50% |
| Eyes (returning home) | 200% |
| Tunnel | ~40% |

---

## 12. Frightened Mode (Phase 6)

### 12.1 State Overview

When ZPac eats an energizer, all active ghosts (not in HOUSE, not EYES) transition to FRIGHTENED. ZPac becomes the predator.

**State sequence for an eaten ghost:**
```
SCATTER/CHASE → FRIGHTENED → EYES → ENTERHOUSE → LEAVEHOUSE → SCATTER/CHASE
```

### 12.2 Phase 6.1 — Frightened State Base

Implemented first: blue sprites, slowdown, timer with expiry, global reversal.

- On energizer trigger: all non-HOUSE ghosts receive `state = GHOST_STATE_FRIGHT`
- Sprites replaced with frightened tiles (palette group PAL_FRIGHTENED = 6, blue)
- Speed reduced to `SPD_GHOST_FRIGHT_L1` (~50% of normal)
- LFSR PRNG for random target generation at intersections (authentic random direction)
- `fright_timer` counts frames, expiry returns to SCATTER/CHASE
- The global scatter/chase timer is **paused** during frightened

### 12.3 Phase 6.2 — Comprehensive AI Fix

This phase resolved a critical architectural bug discovered during testing:

**Double-processing bug:** ghost direction logic was being executed twice per tile center — once in the look-ahead function and once in movement. The result was that ghosts followed incorrect paths systematically.

**Fix:** restructuring the movement loop to ensure the directional decision occurs exactly once per tile center.

**Alignment with floooh for frightened mode:** replaced the PRNG-per-direction approach with the reference method: random target (random tile in the maze) + standard distance² pathfinding. The resulting behavior is correct for the arcade.

**Timer recalibration:** timers went through multiple calibration iterations — first for emulator performance, then for 75Hz real hardware. All final values are calibrated at 75fps (see §11.6 and §17.3).

### 12.4 Phase 6.3 — Ghost Eating + EYES + Flash

Final step: ZPac as predator, eyes returning home, end-of-frightened flash.

**Ghost eating:**
- Collision ZPac with FRIGHTENED ghost → ghost "eaten"
- Brief freeze (~1s) at the moment of eating for visual effect
- Progressive score per energizer: 200 → 400 → 800 → 1600 (doubles per ghost)
- Ghost-per-energizer counter resets on each new energizer

**EYES state:**
- Eaten ghost transitions to `GHOST_STATE_EYES`
- Rendering: tile from frame F0 of the normal ghost + `PAL_EYES` (palette group 8)
- High speed (`SPD_GHOST_EYES_L1` ≈ 2× normal)
- Target: tile (14, 11) — above the ghost house door
- Standard distance² pathfinding, but can cross the door and ignores red zones
- No collision with ZPac (eyes are harmless)

**ENTERHOUSE state:**
- On reaching tile (14, 11), ghost transitions to `GHOST_STATE_ENTERHOUSE`
- Forced downward movement (ignores normal collision logic)
- Destination: ghost's original slot in the ghost house
- On reaching slot → transitions to `GHOST_STATE_LEAVEHOUSE`

**Frightened flash:**
- In the last ~38 frames of the frightened timer (~0.5s at 75fps), rendering alternates
  the palette between PAL_FRIGHTENED (blue, group 6) and PAL_FRIGHT_BLINK (white, group 7)
  every ~8 frames (~4.7 blinks/sec, per the fright_flash_table introduced in Phase 11b)
- The flash is implemented in the renderer as a pure palette swap — zero extra tiles

### 12.5 Phase 6.4 — Level Completion

When `dots_remaining` reaches zero, the level is complete.

**Level complete sequence (arcade-accurate):**
1. ZPac and ghost movement freezes immediately
2. The maze flashes blue/white for ~2 seconds (alternating maze palette group)
3. All sprites (ZPac and ghosts) are hidden during the flash
4. Full reset: `dots_remaining` restored to 244, logic map reinitialized,
   visual maze tiles restored with all dots, actors repositioned to starting coordinates
5. The level restarts with incremented level counter, applying the appropriate speed tier and fruit type for the new level

**Implementation:**
- Check `dots_remaining == 0` in the game loop, after every dot/energizer consumption
- Variable `level_complete_timer` manages the freeze and blink duration
- Maze palette alternates between group 0 (normal blue) and a temporary group (white)
  every ~4 frames via `gfx_tilemap_place()` on layer1 tiles
- Existing `dots_init()` and `ghosts_init()` reused for the reset

### 12.6 Speed Differentiation (Full Recap)

| State | Constant | Approx Value | % Normal |
|---|---|---|---|
| Scatter/Chase normal | SPD_GHOST_NORMAL_L1 | — | 75% |
| Frightened | SPD_GHOST_FRIGHT_L1 | — | 50% |
| Eyes (returning) | SPD_GHOST_EYES_L1 | — | ~200% |
| Tunnel | SPD_GHOST_TUNNEL_L1 | 256 | ~40% |

---

## 13. Audio System (Phase 7)

### 13.1 Infrastructure — sound.c / sound.h

Audio is implemented in the `src/sound.c` / `src/sound.h` module. The module uses PSG hardware registers directly via the `zvb_sound.h` SDK and `snd_bank_save()`/`snd_bank_restore()` to isolate audio writes from the rest of rendering.

Two-voice architecture during gameplay:

| Voice | Use |
|------|-----|
| VOICE0 | Background siren (normal gameplay) / Fright siren (frightened) |
| VOICE1 | Waka-waka (eating dot), Ghost eaten |
| VOICE0+VOICE3 | Death jingle (one-shot sequence) |

`sound_update()` is called every frame in the game loop, before `dots_update()`.

### 13.2 Siren (Step 7.2)

Pattern from the arcade reference: a voice whose frequency oscillates between two values following a 16-step ramp. Each step lasts ~2 frames, producing a fast "uauauaua". Implemented on VOICE0 with sawtooth waveform. The full cycle is ~1 second.

Parameters calibrated on hardware at 75fps (step rates ×0.8 applied in Phase 11):

```c
#define SIREN_FREQ_BASE   0x00C0   /* ~195 Hz */
#define SIREN_FREQ_TOP    0x0180   /* ~390 Hz */
#define SIREN_STEP        0x0010
#define SIREN_STEP_FRAMES 2
```

The siren starts automatically at the beginning of gameplay and stops on death and level complete. It resumes after respawn and after the maze blink.

### 13.3 Waka-waka (Step 7.3)

Two alternating tones (eatdot1/eatdot2) on VOICE1, 50% square waveform. Each call to `sound_waka()` picks the opposite tone from the previous one via a toggle flag. The voice silences automatically after `WAKA_DURATION` frames (brief one-shot).

Registers are written directly (no SDK hold/unhold intermediary) to avoid artifacts.

### 13.4 Fright Siren (Step 7.4)

Replaces the siren on VOICE0 during frightened mode. Pattern derived from behavioral analysis of the arcade fright siren: base frequency rises by a fixed step each frame for 7 frames, then resets — 8-frame cycle (~0.11s at 75fps), an urgent "whooop-whooop-whooop".

Calibrated frequencies (×4 factor vs. direct arcade conversion to compensate for waveform differences):

```c
#define FRIGHT_FREQ_BASE  0x00D0
#define FRIGHT_STEP       0x00D0
#define FRIGHT_CYCLE      8
```

Automatically returns to the normal siren when `fright_countdown` expires.

### 13.5 Ghost Eaten Sound (Step 7.5)

Brief descending sweep on VOICE1 when ZPac eats a ghost. Triangle voice. One-shot of ~6 frames, may overlap with waka, self-silencing.

### 13.6 Death Jingle (Step 7.6 + micro-fix)

Pre-compiled sequence of 90 ticks derived from behavioral analysis of the arcade death sound, converted from WSG to Zeal PSG. Occupies VOICE0 and a noise voice during ZPac's death animation.

The jingle lasts exactly 90 ticks (~1.2 seconds at 75fps). A micro-fix was applied to silence a high-frequency "blip" on the last tick (WSG→PSG conversion artifact): the last entry in the table is forced to zero frequency before releasing the voice.

**Technical note — Sample Table abandoned:** In the initial phase, an attempt was made to use the sample table voice for the jingle (superior timbral quality). The attempt produced white noise due to a conflict between `zvb_gfx.lib` and the audio FIFO controller when both libraries are linked. After empirical verification, the problem was attributed to the graphics controller initialization altering shared registers. The sample table remains disabled; the pure PSG jingle is acceptable and rhythmically correct.

### 13.7 "READY!" Prelude (Step 7.1 / revision)

The arcade prelude (startup jingle on the "READY!" screen) is implemented as a two-voice PSG sequence (VOICE0 sawtooth bass + VOICE1 square melody). The final version uses `gfx_wait_vblank`/`gfx_wait_end_vblank` in the loop to synchronize notes to the frame rate.

One note was slightly out of tune compared to the original: the last 6 bytes of the `prelude_data[]` table were corrected to `0x70` after listening verification against MAME.

---

## 14. Polish and Levels (Phase 8)

### 14.1 Lives System and GAME OVER (Step 8.1)

3-lives system. Variable `lives` (uint8_t, initial = 3) decremented on each death. If `lives == 0` after a death: transition to GAME OVER state with a freeze screen and blinking "GAME OVER" text for ~3 seconds, then full game reset.

The GAME OVER text appears in the maze's center row using the existing font tile system.

### 14.2 Lives HUD (Step 8.2)

Remaining lives are displayed in the bottom bar as ZPac sprite icons (2×2 composite hardware, same scheme as gameplay sprites). Final calibration: `py=456`, inter-icon gap = 16px. Updated on each death and each extra life.

Sprites used: the same HW sprite slots reserved for fruit (slots 44–47 etc.) are shared via contextual reassignment.

### 14.3 Death Animation (Step 8.3)

12-frame ZPac death animation using pre-existing tiles in the tileset (death frames already present from the Phase 2 asset pipeline). On death trigger: movement and ghosts freeze, the animation plays at ~4 frames/step, then transitions to respawn or GAME OVER.

### 14.4 Death Jingle (Step 8.4)

Already documented in §13.6. Integration in the game loop: `sound_play_death()` is called at the start of the death animation, auto-silences at the 90th tick.

### 14.5 Extra Life (Step 8.5)

Extra life granted on reaching 10,000 points (first threshold, as in the arcade). Flag `extra_life_given` prevents double awards. `lives++` with immediate HUD update.

**Collateral fix:** removed from `render_hud()` a residual "00" ghost rendering that was causing artifacts on the score display.

### 14.6 Fruit Bonus (Step 8.6)

Dedicated module `src/fruit.c` / `src/fruit.h`. Fruit appears in the center of the maze twice per level (after 70 and 170 dots eaten). Fruit type determined by current level (cherry L1, strawberry L2, etc.). ~10-second timeout, then disappears.

ZPac/fruit collision: score from 100 to 5000 according to the arcade scoring table. Score popup as a sprite centered on the fruit position, visible for ~2 seconds.

Eat-fruit sound: brief ascending sweep on VOICE1, matching arcade behavior.

### 14.7 Fruit HUD (Step 8.7)

The last 7 fruits encountered during a game appear in the bottom bar on the right as sprite icons. Sprite slots 44–63 (20 sprites = 5 fruits × 4 HW sprites per 2×2 composite). Icons scroll right-to-left as new fruits appear.

### 14.8 Cruise Elroy (Step 8.8)

Blinky progressively speeds up when remaining dots drop below a threshold. 4-tier table per level:

```c
/* { dots_threshold_elroy1, speed_elroy1,
     dots_threshold_elroy2, speed_elroy2 } */
static const elroy_params_t elroy_table[] = {
    { 20, SPD_ELROY1_L1, 10, SPD_ELROY2_L1 },  /* L1  */
    { 30, SPD_ELROY1_L2, 15, SPD_ELROY2_L2 },  /* L2  */
    /* ... */
};
```

Tunnel speed for Blinky in Cruise Elroy: no tunnel speed reduction (as in the arcade).

### 14.9 Scatter/Chase Timing per Level (Step 8.9)

Three distinct timing tables: level 1, levels 2–4, levels 5+. Each table defines scatter and chase phase durations in frames at 75fps, faithful to the arcade original. After phase 4 of the cycle, all ghosts remain in permanent chase.

**Sprite X-offset bug diagnosed and resolved:** during Phase 8 a −10px misalignment was identified on sprites (ZPac and ghosts appeared shifted left). Resolved with an explicit −10px diagnostic test that confirmed the residual offset; corrected in the sprite rendering code.

**VM → native Linux migration (between Phase 7 and 8):** migration from the VirtualBox environment to native Linux revealed that the emulator was running at ~¼ arcade speed under VM. Post-migration, performance recovered; ZPac was subsequently recalibrated first for emulator speed, then definitively for 75Hz real hardware in Phase 11. Intermediate fixes included: `fright_dur` promoted to uint16_t, `anim_tick >> 2` for ZPac's mouth animation (from `>> 1`).

---

## 15. Title Screen and Attract Mode (Phase 9)

### 15.1 Full State Machine

Phase 9 transformed ZPac from a tech demo that launched directly into gameplay into a true arcade experience with an attract loop. The implemented state machine:

```
TITLE → (SPACE pressed with credits) → PLAYING
PLAYING → (game over) → GAME_OVER → (timeout) → TITLE
TITLE → (attract timeout) → DEMO → (demo end) → TITLE
```

States are managed by functions with a consistent signature: `state_xxx_enter()` / `state_xxx_update()` / `state_xxx_exit()`.

### 15.2 Title Screen

The title screen shows the four ghosts with an animated sequential reveal: each ghost appears one at a time with its own large 2×3 sprite on the tilemap (tiles on layer0/layer1, not hardware sprites), accompanied by its name ("BLINKY", "PINKY", "INKY", "CLYDE") in font tiles. The 2×3 tiles (32×48px) for the intro ghosts are dedicated in the tileset.

### 15.3 Coin/Credit System

- Key `1` → inserts coin, increments credits, plays coin sound (`sound_fruit_eaten()` chosen because an isolated `sound_waka()` without rhythmic context sounds like a mere beep)
- `SPACE` → starts game if credits > 0
- Credits displayed in the bottom HUD

**Bug resolved:** the coin sound triggered during the demo→title transition was immediately killed by `sound_stop_all()` in `state_title_enter()`. Solution: `pending_coin_sound` flag that defers play to the frame after the enter.

### 15.4 Attract Mode (Demo)

Arcade chase animation: ZPac chased by 4 ghosts scrolls along a pre-set path in the maze. Mid-way, ZPac eats an energizer, ghosts turn blue, ZPac reverses and eats the ghosts showing score popups 200/400/800/1600. Demo runs with 1 life (not 3).

The GAME OVER text remains visible throughout the entire demo (not cleared on demo mode entry).

### 15.5 Demo AI

Autoplay uses distance-squared navigation (same algorithm as the ghost AI) to follow maze corridors. The ghost danger radius was calibrated to **1 tile** (immediate adjacency): a 4-tile Manhattan radius paralyzed ZPac at intersections.

### 15.6 High Score

In-RAM high score system with session persistence. The maximum score is shown in the top HUD row with the label "HIGH". Does not persist across restarts (no non-volatile storage available in the current flow).

### 15.7 Phase 9 Collateral Fixes

- Removed diagnostic `−10px` offset on the sprite X-axis left over from Phase 8
- Added double-quote (`"`) support to the `put_text()` function for HUD text
- GAME OVER text in white (`PAL_FONT`) is the only practical option: all palette groups have white at color index 1 where font pixels render. A runtime swap on palette group 10 corrupted Clyde's colors (group sharing).

---

## 16. Intermission and Level 256 (Phase 10)

### 16.1 Timing Calibration (pre-Phase 10)

Before the cutscenes, an audio/speed recalibration was performed via MAME benchmarking:

| Parameter | MAME arcade | ZPac pre-fix | ZPac post-fix |
|---|---|---|---|
| Prelude jingle | 4.36s | 5.63s | ~4.36s ✅ |
| Corridor traversal | 3.78s | 4.26s | ~3.76s ✅ |

**Prelude:** the jingle was playing 245 ticks with VBlank frame as clock. Accumulator increased to `acc=119` (factor ×1.291) to bring the time to 4.36s. **Gameplay speed:** factor ×1.127 applied to all speed tables in `zpac_types.h`, `game_loop.c`, `ghost.h`, `ghost.c`. Resulting corridor: 3.76s vs 3.78s arcade (−0.5%, arcade-perfect).

### 16.2 Level 256 Split-Screen Bug (Step 10.1)

Faithful implementation of the arcade Level 256 bug. In the original, level 256 causes screen corruption: the right half of the maze is overwritten with random tiles derived from the level counter overflow (8-bit), making the level impossible to complete.

Dedicated module `src/level256.c` / `src/level256.h`. The corruption logic was reconstructed by studying the documented Z80 overflow behavior and published arcade technical references, then replicating the resulting tile/color pattern. The right half shows 8 real (walkable) dots; everything else is walkable corridor but without logical dots — the level is impossible to complete, as in the original.

> ⚠️ A debug shortcut (`game_level=1, dot_count=10` in `game_playing_init`) was used during development for quick trigger testing and has since been removed.

### 16.3 Cutscene Act 1 (Step 10.2)

Module `src/cutscene.c` / `src/cutscene.h` with the cutscene framework + Act 1.

**Act 1 — "They Meet":** ZPac and Blinky run leftward across the screen (Phase 1). Then a Big ZPac (48×48px) and a frightened ghost run rightward (Phase 2). Big ZPac uses 27 new tiles (indices 357–383, 3×3 grid × 3 frames) generated mathematically as a circle with arcs for the mouth — smooth curves, not nearest-neighbor upscale. Script `generate_big_pacman_v2.py` with sin/cos for outlines.

**Tileset:** now at 384/512 tiles (128 remaining). `TOTAL_TILES=384`, `TILESET_SIZE=49152`. The `zpac_tiles.bin` file must be copied to the emulator's HostFS path after any tileset change.

### 16.4 Cutscene Acts 2 and 3

Completed within the cutscene framework. Act 2 ("The Chase") and Act 3 ("They Argue") implement the standard arcade choreographies: ghost-ZPac confrontation, "naked ghost" animation (the wiggling worm), argument. The arcade worm is **horizontal**: pink head with eyes on top, red body/tail trailing behind — not a standing little ghost.

### 16.5 Intermission Music

The intermission melody was reconstructed from publicly documented arcade music references and verified against a reference MIDI. The two musical parts (melody + bass with loop) were converted to the Zeal PSG tick-by-tick format using the same approach as the prelude.

### 16.6 Binary Size — Critical Constraint

With Act 2, the binary reached **47,014 bytes** (limit ~48 KB), triggering a dedicated optimization phase (Phase 11b) that brought it back to 45,928 bytes.

Key constraints identified:
- `zpac_palette` declared `static const` in the header → every `.c` that references it adds 512 bytes to its translation unit. **Rule:** do not reference `zpac_palette` from `cutscene.c`.
- Tick-by-tick music array (1316 bytes raw) compressed with RLE → **177 bytes** with minimal decoding overhead.

---

## 17. Hardware Port and Final Calibration (Phase 11)

### 17.1 First Run on Real Hardware

Phase 11 marked the transition from emulator to physical Zeal v1.2.0 + Video Board v1.1. Before ZPac could run, two firmware updates were required: the bootloader from v1.0.0 to v1.3.1 (applied via UART with a USB-to-TTL adapter), and the FPGA firmware from v0.1.0 to v1.0.0 (required for `gfx_tileset_load()` to function — tileset memory was read-only in earlier versions). Both updates were performed successfully.

ZPac ran on real hardware on the first attempt after firmware update.

### 17.2 Tileset Path and File Rename

The emulator loads the tileset from HostFS (`H:/zpac_tileset.bin`), which does not exist on physical hardware. The loader was updated with a dual-path fallback: it first tries a local path (`zpac_tiles.bin`, for real hardware via ZealFS), then falls back to `H:/zpac_tiles.bin` (emulator). At the same time the tileset file was renamed from `zpac_tileset.bin` to `zpac_tiles.bin` and the executable from `zpac_test.bin` to `zpac.bin`.

### 17.3 75Hz VBlank Calibration

A critical discovery: real Zeal hardware running VESA 640×480 operates at **75Hz VBlank**, not 60Hz. The emulator had been running at 60Hz. This meant all gameplay speeds and frame-count timers were calibrated for the wrong rate.

Benchmark measurement on hardware (corridor traversal: 2.98s measured vs 3.76s MAME reference, ratio ≈ 0.8 = 60/75) confirmed the discrepancy. The fix was systematic across all six files containing timing constants:

- All gameplay speeds: multiplied by **×0.8** (60/75)
- All frame-count timers: multiplied by **×1.25** (75/60)
- Audio: prelude accumulator adjusted from 119 to 95; siren/waka/fright step rates ×0.8, durations ×1.25
- A uint16_t overflow for long chase durations (values exceeding 65,535) was resolved by capping at 65,535 — negligible at ~14.5 minutes of uninterrupted play

Files updated: `zpac_types.h`, `ghost.h`, `ghost.c`, `game_loop.c`, `fruit.c`, `cutscene.c`.

Post-calibration benchmark: corridor traversal 3.76s vs 3.78s MAME reference (−0.5%, arcade-perfect).

### 17.4 SNES Controller

The SNES gamepad adapter (connected to Z80 PIO Port A at 0xD0/0xD2) was implemented entirely within `input.c` using SDCC's `__sfr __at` direct port I/O — no SDK layer required. D-pad maps to directions; Select triggers coin insert (edge detect); Start triggers game start (edge detect). Both keyboard and SNES controller remain active simultaneously; the SNES d-pad overrides keyboard direction input.

An anti-noise guard handles the emulator: if all 12 input bits read as pressed simultaneously (0x0FFF — physically impossible), the input is treated as no controller present.

### 17.5 Audio Bug (Unresolved)

After eating fruit during gameplay, the waka-waka sound plays only on the left channel for the remainder of the session. Root cause: writing to `zvb_peri_sound_master_vol` or `zvb_peri_sound_hold = 0` during active gameplay corrupts the right channel stereo state — a known ZVB SDK hazard that manifests differently in the fruit-eat code path than in the coin-sound path (where a separate `sound_coin()` function was already isolated for this reason).

The bug was being investigated using a stale binary, which invalidated several test iterations before being caught. The decision was made to restore from the last known-good snapshot (post-SNES controller implementation) and restart debugging with the correct binary. Resolution is pending.

### 17.6 SD Card

The TF card (microSD) consistently fails to initialize with error `TFC0 init error $FD`. ZealOS uses ZealFS v2, not FAT32, requiring the Zeal Disk Tool for proper formatting. A SanDisk SDHC 32GB card was tested and failed; compatibility with the specific card model is under investigation via the Zeal 8-bit Discord community.


## 18. Timing Audit and Binary Size Optimization (Phase 11b)

### 18.1 Context

During post-Phase 11 tests on real hardware, the speed increase between level 1 and level 2 felt noticeably too sharp. Investigation revealed systematic errors in the speed tables: the ×1.127 calibration factor had been applied to a wrong base value (307 instead of the correct 433), producing cascading inaccuracies across all speed parameters. A full timing audit was conducted comparing every value against the Pac-Man Dossier Table A.1 and the floooh reference implementation.

Correcting the timing tables caused the binary to grow back toward the 48 KB limit, requiring a structural code optimization pass to recover space.

**Result:** binary reduced from 47,014 to **45,928 bytes**, despite adding new tables and logic.

### 18.2 Speed Table Fix

**Files:** `game_loop.c`, `zpac_types.h`

Only `pac_normal` had been correctly calibrated via MAME benchmarking. All other values had been derived from the wrong base. The correct formula is:

```
speed_value = 433 × arcade_percentage
```

Worst errors before the fix:
- `ghost_fright`: −20% (frightened ghosts too slow)
- `ghost_tunnel`: −34% (tunnel almost impassable)
- `pac_fright` L5–20: +6.7% (faster than `pac_normal` — physically nonsensical)

All four speed tiers (L1, L2–4, L5–20, L21+) corrected with all six parameters each.

### 18.3 Elroy Table Expansion

**File:** `ghost.c`

The original Cruise Elroy implementation used 4 generic tiers instead of the 9 distinct per-level combinations in Dossier Table A.1. Updated to `elroy_table[9]` with per-level selection. L21+ corrected from 90%/95% to 100%/105% (values 433/455).

### 18.4 Ghost House Dot Limits per Level

**File:** `ghost.c`

All levels were using L1 limits (Pinky=0, Inky=30, Clyde=60). Replaced with a `dot_limit_table[3][4]` covering three groups:

| Group | Pinky | Inky | Clyde |
|---|---|---|---|
| L1 | 0 | 30 | 60 |
| L2 | 0 | 0 | 50 |
| L3+ | 0 | 0 | 0 (immediate exit) |

### 18.5 Frightened Flash Count per Level

**Files:** `game_loop.c`, `zpac_types.h`, `ghost.c`

`FRIGHT_FLASH_FRAMES` was a fixed constant (75 frames) and the flash toggle rate (`>> 2`) was too fast. Replaced with a `fright_flash_table[21]` (5 flashes = 80 frames for L1–8, 3 flashes = 48 frames for L9+) and the toggle slowed to `>> 3` (~4.7 blinks/sec, arcade-like). The constant was replaced by the function `get_fright_flash_frames()`.

### 18.6 Cutscene Timing for 75fps

**File:** `cutscene.c`

Cutscene sprite movement speeds had never been adapted from 60fps to 75fps — animations were running ~25% faster than the intermission music. Fix: a `cs_skip` counter (uint8_t, 0–4) skips movement 1 frame in 5, producing 60 effective movement steps/sec. Inter-phase pauses scaled ×1.25.

A first implementation using `tick % 5` (16-bit division) caused a stack overflow crash. The second version using an 8-bit counter has a negligible footprint.

### 18.7 Binary Size Optimizations

| # | Intervention | File | Saving |
|---|---|---|---|
| A | Dead code removal | `main.c` | 464 B |
| B | `maze_ascii` bitpack compression | `maze_logic.c` | 660 B |
| C | `arcade_colors` nibble pack | `level256.c` | ~230 B |
| D | Procedural death sound | `sound.c` | ~130 B |
| E | `pow10` table dedup via extern | `dots.c`, `game_loop.c` | ~28 B |
| | **Total** | | **~1,512 B** |

**A — Dead code:** `set_sprite_2x2()` and `render_maze()` in `main.c` were never called at runtime.

**B — Maze compression:** the `maze_ascii[]` array (899 bytes of char data) was replaced with `maze_packed[217]` (2 bits per cell) plus `energizer_pos[4][2]` (8 bytes for the 4 energizer positions).

**C — Level 256 palette:** the `arcade_pal_packed[217]` nibble-packed array replaced the original 434-byte table; `map_arcade_to_zpac_palette()` was removed as it became dead code.

**D — Death sound:** `death_snd_vol[90]` replaced by a procedural `death_get_vol()` function; `death_snd_wave[90]` removed (was already dead code).

**E — pow10 dedup:** the `pow10[]` lookup table was declared as `extern` and shared between `dots.c` and `game_loop.c`.

### 18.8 Validation

All tests performed on real hardware using the level-select shortcut in `game_playing_init()`. Test shortcut removed after completion.

| Test | Trigger | Result |
|---|---|---|
| Level 256 | `game_level=254` | Right-half corruption visible, 8 dots, intentionally unbeatable ✅ |
| Cutscene Act 1 | `new_level==2` | ZPac+Blinky left, pause, Blinky blue + Big ZPac right, music sync ✅ |
| Cutscene Act 2 | `new_level==5` | Nail, tear, Blinky shock+look right, music sync ✅ |
| Cutscene Act 3 | `new_level==9` | Patched Blinky, naked ghost worm, music sync ✅ |


## 19. Audio: Architecture and Strategy (Technical Reference)

### 19.1 The Platform Gap

The original arcade audio has a unique character given its wavetable synthesis: custom waveforms reproduced via a WSG chip with 3 voices and 16 volume levels. These waveforms — not pure sinusoids but curves with particular harmonics — give the game its unmistakable timbre.

Zeal has a PSG with 4 standard voices (square, triangle, sawtooth, noise) plus a 256-byte sample voice, with only 4 volume levels.

### 19.2 Approximation Strategy

90% of recognizing the arcade sounds depends on the pattern of frequencies and rhythm, not the exact timbre. The approximation strategy:

| Original Sound | Zeal Approximation | Voice |
|---|---|---|
| Siren/power pellet | Triangle wave with frequency sweep | 0 (always active) |
| Waka-waka (eating dot) | Square wave alternating two notes | 1 |
| Eat ghost, fruit, death | Square/triangle one-shot | 2 |
| Intro jingle, extra life | Programmed note sequence | 3 |

The sample voice (5th) could be used for special effects where timbre is critical, but the 256-byte buffer limitation makes it suitable only for very short sounds.

### 19.3 Relevant Audio Registers

```
Frequency formula: freq_hz = (sample_rate × reg_value) / 2^16
Where sample_rate = 44100 Hz

Inverse conversion: reg_value = (freq_hz × 65536) / 44100

Waveform:  0 = square, 1 = triangle, 2 = sawtooth, 3 = noise
Volume:    4 levels (0%, 25%, 50%, 75%, 100%)
Hold:      bitmap for atomic mute/unmute of multiple voices
```

---

## 20. Technical References Used

Reference materials are used to *understand* the target behavior — not as code to port. For every system (ghost AI, speed, audio, rendering) the "what" is studied from documentation and published analyses; the "how" is designed specifically for the Zeal.

### 20.1 Project Documentation

| File | Content | Role |
|---|---|---|
| `ZPac_Game_Design_Document_v3.1.md` | Complete architecture, technical specs, phase plan | Master document |
| `ZPac_Reference.md` | Session log, lessons learned, quick reference | Technical diary |
| `ZPac_analisi_pacman_c_floooh.md` | Analysis of floooh's pacman.c clone | Reference patterns |
| `pacman_flooh_reference.c` | Full source of the reference clone | Reference code |

### 20.2 Original Arcade Game Documentation

| File | Content | Relevance |
|---|---|---|
| `The_PAC_Dossier.pdf` | Complete gameplay guide, ghost AI, speed | ⭐⭐⭐ Behavior bible |

### 20.3 Zeal Platform Documentation

| File | Content |
|---|---|
| `zeal_8bit_inside.pdf` | Complete Zeal architecture: CPU, video, audio, I/O |
| `Zeal_Video_SDK_readme.md` | Graphics API: tilemap, sprites, palette |
| `Zeal_OS_readme.md` | ZealOS syscalls, filesystem, drivers |
| `Zeal_game_dev_kit.md` | ZGDK: sprite arena, input, tilemap, collision |
| `zvb_gfx.h`, `zvb_hardware_h.asm` | SDK headers with registers and constants |
| `zos_sys.h`, `zos_video.asm`, `zos_keyboard.asm` | OS API and drivers |

### 20.4 Zeal Reference Code

| File | Content | What We Learned |
|---|---|---|
| `snake.c` | Snake game for Zeal | Game loop patterns, VBlank sync, non-blocking input |
| `controller.c/h` | SNES adapter driver for Z80 PIO | Gamepad input via PIO Port A, D-pad → arrow keys |
| `title.c/h` | Z-Fighter title screen | State machine, font rendering |
| `ZPac_analisi_ZGDK.pdf` | ZGDK framework analysis | Sprite architecture, tilemap, collisions |
| `ZPac_analisi_ZFighter.pdf` | Z-Fighter game analysis | Multi-file patterns, game states |

---

## 21. Development Chronology

### 21.1 Work Sessions

| # | Date | Session | Key Result |
|---|---|---|---|
| — | 18/02 | Environment setup | Ubuntu configured, emulator working, SDCC+CMake OK |
| — | 18/02 | Scaffolding | First hello world: yellow tile on blue background, keyboard input validated |
| — | 19/02 | Asset strategy | Full asset pipeline plan, source URLs, conversion tools |
| — | 22/02 | floooh analysis | pacman.c analyzed: trigger system, ghost AI, movement patterns |
| — | 22/02 | Mode 5 analysis | Discovery: 329 tiles > 256 Mode 5 limit → Mode 6 decision |
| 1 | 21/02 | Asset download | Pipeline plan, graphic and audio source URLs |
| 2 | 21/02 | Asset cleanup | JPEG artifact cleanup, color palette reconstructed from arcade documentation |
| 3 | 22/02 | Sprite extraction | 66 16×16 sprites reconstructed from arcade documentation |
| 4 | 22/02 | Maze + font reconstruction | 79 maze tiles + 40 font chars reconstructed |
| 5 | 22/02 | PNG → ZVB converter | `zpac_gfx.h` generated with full pipeline |
| 6 | 22/02 | Documentation | zpac_gfxsrc_reference.md created |
| 7 | 22/02 | Maze test + debug | First maze render on emulator (artifacts) |
| 8 | 22/02 | Wall alignment | Root cause: wireframe vs solid tile — need solid tile data |
| 9 | 22/02 | Maze encoding analysis | ASCII maze structure analyzed from floooh reference |
| 10 | 22/02 | Maze reconstruction | Maze geometry reconstructed from arcade documentation, 85 solid tiles |
| **11** | **22/02** | **Phase 1 complete** | **✅ 1.5× maze on emulator, Mode 6 confirmed** |
| 12 | 22/02 | Sprite strategy | 2×2 composite decision (Option F) |
| 13 | 23/02 | Tile budget analysis | 438/512 estimated → 329/512 after dedup |
| **14** | **23/02** | **Phase 2 complete** | **✅ Sprites + font + HUD on emulator, 329/512 tiles** |
| 15 | 23/02 | Memory analysis | Options: external file, RLE, bank switching, flip dedup |
| 16 | 23/02 | External tileset | Streaming approach, discovery: `-u` without filesystem |
| 17 | 23/02 | Phase 3 planning | 5-step plan, logic map design |
| **18** | **23/02** | **Memory milestone** | **✅ HostFS working, external tileset, binary ~7KB** |
| 19 | 23/02 | Step 3.1 | Logic map + data structures implemented |
| 20 | 23/02 | Step 3.2 prep | Self-contained prompt for game loop + input |
| 21 | 24/02 | Audio analysis | WSG vs PSG comparison, approximation strategy |
| 22 | 24/02 | Audio test prompt | Prompt for 440Hz scale test on emulator |
| 23 | 25/02 | Step 3.2 | Game loop + sprite calibration + base movement ✅ |
| 24 | 25/02 | Step 3.3 | ZPac animation 4 frames × 4 directions ✅ |
| **25** | **25/02** | **Step 3.4** | **✅ Dot eating with deferred tile replacement, eating pause** |
| 26 | 25/02 | Step 3.5 | Tunnel wrap-around ✅ |
| 27 | 02/03 | Phase 3 perf | Bottleneck ~21fps: tile+sub refactor, fast_wait_vblank |
| **28** | **02/03** | **Phase 3 complete** | **✅ Smooth emulator performance, arcade-accurate movement, score HUD** |
| 29 | 02/03 | Phase 4 step 4.1 | Ghost struct + base movement |
| 30 | 02/03 | Phase 4 step 4.2 | Ghost sprite rendering 2×2 composite |
| 31 | 02/03 | Phase 4 step 4.3 | Game loop integration |
| 32 | 02/03 | Phase 4 step 4.4 | Wall collision + tunnel wrap for ghosts |
| 33 | 02/03 | Phase 4 step 4.5 | Scatter AI with distance² pathfinding + tie-breaking |
| 34 | 02/03 | Phase 4 step 4.6 | Scatter/chase timer + global reversal |
| 35 | 02/03 | Phase 4 step 4.7 | Individual chase targeting (Blinky/Pinky/Inky/Clyde) |
| **36** | **02/03** | **Phase 4 complete** | **✅ ZPac↔ghost collision, scatter/chase working** |
| 37 | 02/03 | Phase 5 step 5.1 | Ghost house state machine + bouncing |
| 38 | 02/03 | Phase 5 step 5.2 | Individual dot counter for exit ordering |
| 39 | 02/03 | Phase 5 step 5.3 | Exiting path (ghost house → maze) |
| 40 | 02/03 | Phase 5 step 5.4 | Anti-stall timer |
| 41 | 02/03 | Phase 5 step 5.5 | Frightened mode + energizer |
| 42 | 02/03 | Phase 5 step 5.6 | Ghost eyes (returning home) |
| 43 | 02/03 | Phase 5 step 5.7 | Ghost-eating score chain (200/400/800/1600) |
| 44 | 02/03 | Phase 5 step 5.8 | Speed differentiation by state |
| **45** | **02/03** | **Phase 5 complete** | **✅ Complete ghost AI, frightened/eyes, score chain** |
| 46 | 02/03 | Phase 6.1 | Frightened State Base: blue sprites, PRNG, timer, reversal |
| 47 | 02/03 | Phase 6.2 | AI Fix: double-processing bug, floooh alignment, timers recalibrated for emulator |
| 48 | 04/03 | Phase 6.3 | Ghost eating + EYES + ENTERHOUSE + frightened flash |
| **49** | **04/03** | **Phase 6 complete** | **✅ Full frightened mode: eating, progressive score, eyes, flash** |
| 50 | 04/03 | Phase 6.4 | Level completion: dots_remaining==0 check, maze blink, level reset |
| **51** | **04/03** | **Phase 6 complete** | **✅ Full game loop: level completion with blink and reset** |
| 52 | 05/03 | Zeal Music Tracker test | Audio hardware test with external project: emulator audio confirmed |
| 53 | 05/03 | Phase 7 step 7.1 | sound.c/sound.h infrastructure + 440Hz tone test: PSG confirmed |
| 54 | 05/03 | Phase 7 step 7.2 | Background siren on VOICE0 sawtooth, calibrated via listening feedback |
| 55 | 05/03 | Phase 7 step 7.3 | Waka-waka on VOICE1 square, alternating toggle, direct register writes |
| 56 | 05/03 | Phase 7 step 7.4 | Fright siren, correct ×4 frequencies after calibration, 8-frame cycle |
| 57 | 05/03 | Phase 7 step 7.5 | Ghost eaten sweep on VOICE1 |
| 58 | 05/03 | Phase 7 step 7.6 | Death jingle 90 ticks from behavioral analysis of arcade audio, final blip micro-fix |
| 59 | 05/03 | Sample table attempt | zvb_gfx vs FIFO audio conflict → abandoned, pure PSG definitive |
| **60** | **05/03** | **Phase 7 complete** | **✅ Full audio: siren, waka, fright, eaten, death jingle** |
| 61 | 05/03 | VM → Linux migration | VirtualBox throttled to ¼ speed, native Linux restored real performance |
| 62 | 05/03 | Timing recalibration | Emulator-phase calibration: speed_tiers adjusted, fright_dur → uint16_t, anim_tick >> 2 |
| 63 | 07/03 | Phase 8 step 8.1 | Lives system + GAME OVER state |
| 64 | 07/03 | Phase 8 step 8.2 | Lives HUD sprite 2×2, calibration py=456 gap=16px |
| 65 | 07/03 | Phase 8 step 8.3 | Death animation 12 frames |
| 66 | 07/03 | Phase 8 step 8.4 | Death jingle integrated + blip micro-fix |
| 67 | 07/03 | Phase 8 step 8.5 | Extra life @10k points + render_hud ghost "00" fix |
| 68 | 07/03 | Phase 8 step 8.6 | Fruit bonus: fruit.c/fruit.h, collision, sound, score popup |
| 69 | 07/03 | Phase 8 step 8.7 | Fruit HUD icons bottom-right, sprites 44-63 |
| 70 | 07/03 | Phase 8 step 8.8 | Cruise Elroy 4-tier + Blinky tunnel speed |
| 71 | 07/03 | Phase 8 step 8.9 | Scatter/chase timing 3 tables per level |
| 72 | 07/03 | X-offset bug fix | −10px sprite misalignment diagnosed and fixed |
| **73** | **07/03** | **Phase 8 complete** | **✅ Full polish: lives, death, fruit, Elroy, level timing** |
| 74 | 08/03 | Phase 9 — State machine | TITLE/PLAYING/GAME_OVER/DEMO state machine + coin/credit system |
| 75 | 08/03 | Phase 9 — Title screen | Animated 2×3 tile ghost intro, sequential per-ghost reveal |
| 76 | 08/03 | Phase 9 — Attract mode | Chase animation + energizer + ghost eating with 200/400/800/1600 popups |
| 77 | 08/03 | Phase 9 — Demo AI | Distance-squared navigation, danger radius calibrated to 1 tile |
| 78 | 08/03 | Phase 9 — High score | In-RAM high score system with HUD display |
| 79 | 08/03 | Phase 9 — Collateral fixes | Removed −10px offset, put_text double-quote fix, coin sound pending flag |
| **80** | **08/03** | **Phase 9 complete** | **✅ Full attract loop, end-to-end arcade experience** |
| 81 | 10/03 | Phase 10 calibration | Prelude acc=119 (4.36s), gameplay speed ×1.127 (3.76s vs 3.78s arcade) |
| 82 | 10/03 | Phase 10 — Level 256 | Split-screen bug simulated: right half corruption from documented overflow behavior |
| 83 | 10/03 | Phase 10 — Cutscene Act 1 | cutscene.c/h framework, mathematical 48×48 Big ZPac, tiles 357-383 |
| 84 | 10/03 | Phase 10 — Cutscene Acts 2/3 | "The Chase" and "They Argue" with naked ghost (horizontal worm) |
| 85 | 10/03 | Phase 10 — Intermission music | Intermission melody reconstructed from arcade documentation, RLE compression 1316→177 bytes |
| **86** | **10/03** | **Phase 10 complete** | **✅ Cutscenes + Level 256 + intermission music, binary 47014B** |
| 87 | 14/03 | Phase 11 — Firmware update | Bootloader v1.3.1 + FPGA v1.0.0 applied; ZPac boots on real hardware ✅ |
| 88 | 14/03 | Phase 11 — Tileset path + rename | Dual-path fallback (local first, H:/ fallback); zpac_tiles.bin + zpac.bin |
| 89 | 14/03 | Phase 11 — 75Hz calibration | Hardware VBlank=75Hz confirmed; all speeds ×0.8, all timers ×1.25 across 6 files |
| 90 | 14/03 | Phase 11 — SNES controller | PIO Port A input via `__sfr __at`, d-pad+Select+Start, anti-noise guard |
| 91 | 14/03 | Phase 11 — Audio bug investigation | Waka left-channel bug: root cause isolated to master_vol write; debugging from clean snapshot |
| **92** | **14/03** | **Phase 11 complete** | **✅ Running arcade-perfect on real hardware; SNES input active; 1 audio bug pending** |
| 93 | 14/03 | Phase 11b — Timing audit | Speed table base error found (307→433); all 4 tiers × 6 parameters corrected |
| 94 | 14/03 | Phase 11b — Elroy + dot limits | elroy_table[9] per-level, dot_limit_table[3] groups |
| 95 | 14/03 | Phase 11b — Flash + cutscene | fright_flash_table[21], cs_skip frame-skip (fixed stack overflow) |
| 96 | 14/03 | Phase 11b — Size optimization | Dead code, maze bitpack, nibble pack, procedural death sound: −1,086 bytes |
| 97 | 14/03 | Phase 11b — Validation | All cutscenes + Level 256 tested via shortcut on hardware; shortcut removed |
| **98** | **14/03** | **Phase 11b complete** | **✅ Binary 45,928 bytes; all timing arcade-accurate; 3.2 KB margin** |

### 21.2 Lessons Learned (Cumulative Log)

**Phase 1:**

- **L1:** PNG sprite sheets from Spriters Resource contain raw ROM data, not arcade output. The complete palette pipeline must be replicated.
- **L2:** The Mode 5 viewport (15 rows) is not enough for the arcade maze layout (31 rows + HUD).
- **L3:** Mode 6 with 1.5× scaling is the optimal solution: 512 tiles and 4bpp.
- **L4:** In Mode 6, layer1 = attributes. Every tile requires writes to layer0 AND layer1.

**Phase 2:**

- **L5:** Sprite quadrant deduplication is very effective (341 tiles vs 438 estimated).
- **L6:** Ghost eyes reuse the normal ghost tile (frame 0) with a different palette.
- **L7:** Frightened flash uses the same tiles as frightened blue with a different palette group.
- **L8:** The 48KB constraint is real. Inline tileset (42KB) leaves no space for gameplay.
- **L9:** Emulator with `-u` alone does not support filesystem. HostFS must be enabled in the kernel.
- **L10:** `gfx_sprite` struct: fields `x`, `y`, `tile`, `flags`. Flags: bits 7-4 = palette, bit 3 = FLIP_X, bit 2 = FLIP_Y, bit 0 = TILE_BIT9.

**Memory Milestone:**

- **L11:** `gfx_tileset_load()` with `from_byte` works for chunk streaming.
- **L12:** HostFS mounts as drive `H:`. Path: `"H:/filename"`. Works with `-u` + `-H`.

**Phase 3:**

- **L13:** Division/modulo on Z80 is prohibitive (~300+ cycles per op). Restructuring coordinates to tile+sub eliminates all divisions and jumps from ~21fps to 60fps.
- **L14:** The SDK's `gfx_wait_vblank()` performs costly MMU operations on every call. A custom `fast_wait_vblank()` with direct I/O read is far more efficient.
- **L15:** The 1.5× scale factor between logical and visual grids creates misalignment in tile replacement. The deferred + on-demand check solution is robust and requires no additional data structures.
- **L16:** Interactive sprite position calibration (X/Y offset via arrow keys) is necessary to achieve correct alignment between sprites and maze.

**Phase 4:**

- **L17:** The `gfx_sprite` struct must be zero-initialized before use. An uncleared HEIGHT_32PX bit causes sprites to be 32px tall instead of 16px.
- **L18:** Distance² pathfinding only works correctly if tie-breaking follows the arcade order: UP > LEFT > DOWN > RIGHT.
- **L19:** uint8_t absolute values for distance² calculations avoid costly int16_t multiplications (~300+ cycles) without losing correctness.

**Phase 5:**

- **L20:** Bouncing in the ghost house must use reduced speed and explicit boundaries — without these, the exit logic corrupts.
- **L21:** The global dot counter (activated after a life is lost) is separate from the individual one and uses different thresholds (Pinky=7, Inky=17, Clyde=32).
- **L22:** Eyes returning home require separate pathfinding that ignores normal restrictions (can enter the ghost house from above).

**Phase 6:**

- **L23:** The ghost double-processing bug is hard to diagnose visually — ghosts move but follow wrong paths. Requires analysis of the movement loop structure.
- **L24:** Timers must be calibrated to the **actual** frame rate of the target platform, not the theoretical one (60fps ≠ 25fps actual on unoptimized Z80).
- **L25:** Frightened flash requires no extra tiles: it is a pure swap between PAL_FRIGHTENED (group 6) and PAL_FRIGHT_BLINK (group 7) every 4 frames.
- **L26:** The EYES state must be able to cross the ghost house door and ignore red zones — exceptions to normal movement logic handled with an explicit flag.
- **L27:** The freeze for level complete requires hiding sprites (not just blocking movement) to prevent characters appearing "frozen" on screen during the maze blink animation. Existing `dots_init()` + `ghosts_init()` are sufficient for the reset without adding new code paths.

**Phase 7:**

- **L28:** Arcade frequencies do not convert linearly to the Zeal PSG when wavetables have multiple cycles per sample. Arcade 32-sample wavetables produce a perceived frequency ~4× higher than the fundamental — target frequencies must be multiplied accordingly.
- **L29:** The PSG sample table conflicts with `zvb_gfx.lib` when both are linked together. The graphics controller alters shared registers that the audio FIFO uses for timing. Workaround: pure PSG for all effects.
- **L30:** Writing PSG registers directly (without the SDK hold/unhold functions) eliminates audible glitches that the hold→write→unhold sequence introduces on already-active voices.
- **L31:** The floooh register dump (snd_func_dead) is the most reliable source for the death jingle. Direct WSG→PSG conversion produces a single high-frequency "blip" on the last tick — it must be zeroed explicitly.

**Phase 8:**

- **L32:** Migrating from VM to native Linux revealed that VirtualBox throttling was completely masking real performance. All game timers must be recalibrated when the execution platform changes.
- **L33:** The `uint8_t` type for `fright_duration` is insufficient at 75fps (max 255 frames = 3.4s). `uint16_t` is needed for higher levels (up to ~600 frames at 75fps).
- **L34:** Fruit score popups as hardware sprites (rather than tilemap tiles) avoid having to alter and restore tilemap logic during transient events.
- **L35:** A systematic sprite offset (−10px on the X axis) can accumulate from multiple independent sources. A diagnostic test with a known offset (explicit −10px) is the fastest method to isolate and measure each source's contribution.

**Phase 9:**

- **L36:** A coin sound triggered during a state transition (demo→title) is killed by `sound_stop_all()` in the new state's enter function. Solution: `pending_coin_sound` flag that defers play to the frame after the transition.
- **L37:** A ghost danger radius of 4 Manhattan tiles in the demo AI paralyzes ZPac at intersections. The correct radius is 1 tile (immediate adjacency).
- **L38:** Red for GAME OVER is not achievable with the current palette system: all groups have white at color index 1 where font pixels render. Runtime swap on palette group 10 corrupts Clyde's colors (group sharing). White PAL_FONT is the practical solution without palette refactoring.

**Phase 10:**

- **L39:** Prelude calibration is done by acting on the accumulator (acc=119) and not on the note table — changing the notes would alter the pitch.
- **L40:** Arcade music formats use proprietary byte sequences for timing and waveform control. Direct format decoding is fragile; cross-checking against a reference MIDI is more reliable and avoids dependency on raw arcade data.
- **L41:** `zpac_palette` declared `static const` in the header adds 512 bytes for every translation unit that references it. With the binary at 47KB/48KB limit, every unnecessary reference from a new `.c` is critical.
- **L42:** Mathematical curves (sin/cos) for large sprites (48×48+) produce much smoother outlines than nearest-neighbor 2× upscale. Offline generation with Python/Pillow is the correct pipeline.
- **L43:** Claude Code can get file paths wrong when working with Python scripts + tileset. Explicit absolute paths in the prompt are mandatory — never use `find`.

**Phase 11:**

- **L44:** Real Zeal hardware runs at 75Hz VBlank (VESA 640×480@75Hz), not 60Hz. All speeds and frame-count timers must be multiplied by 0.8 and 1.25 respectively when porting from emulator. Never assume the emulator frame rate matches hardware.
- **L45:** The ZVB firmware version is critical: `gfx_tileset_load()` requires v1.0.0+. Tileset memory is read-only in v0.1.0 — writes are silently ignored, producing a blank or corrupt display with no error.
- **L46:** Writing `zvb_peri_sound_master_vol` or `zvb_peri_sound_hold = 0` during active gameplay corrupts the right audio channel. The `sound_coin()` and `sound_fruit_eaten()` functions must be kept separate: coin sound (used on title screen) can safely restore master_vol and hold; fruit-eat sound (used during gameplay) must not touch either register.
- **L47:** Debugging audio on hardware requires certainty about which binary is running. A stale binary can invalidate multiple test iterations before the issue is discovered. Always confirm the build timestamp or a known behavior before starting an investigation.
- **L48:** SDCC `__sfr __at(addr)` provides direct Z80 I/O port access without any SDK layer — the correct approach for PIO-based peripherals like the SNES controller adapter.

**Phase 11b:**

- **L49:** Calibrating one speed value correctly against a hardware benchmark (MAME) does not guarantee the others are correct if they share the same base formula. Every speed parameter must be independently verified against the reference table.
- **L50:** The correct base for ZPac speed values is 433 (pixels/second at 75fps), not 307. Any future speed constant must use `value = 433 × arcade_percentage`.
- **L51:** `tick % N` for frame-skip logic uses 16-bit division on Z80 (~300+ cycles) and increases code size. With the binary already near the 48 KB RAM limit, the extra code generated pushed the stack into occupied memory, causing a crash. An 8-bit counter incremented each frame has identical effect, negligible code size, and zero division cost.
- **L52:** Before any binary size optimization, remove dead code first — it yields the highest return for zero risk. `set_sprite_2x2()` and `render_maze()` in `main.c` alone recovered 464 bytes.
- **L53:** Bitpacking lookup tables (2 bits/cell for maze, nibble-pack for palette) is the most effective structural compression for Z80: no runtime decompression overhead beyond a shift/mask, and savings of 50-75% on the original array size.

---

## 22. Current Status and Next Steps

### 22.1 Completed Phases

| Phase | Status | Date | Evidence |
|---|---|---|---|
| Phase 1 — Maze rendering | ✅ | 22/02/2026 | Solid blue walls, 244 dots, ghost house door, centered maze |
| Phase 2 — Sprites, font, palette | ✅ | 23/02/2026 | ZPac + 4 ghosts with correct colors, HUD, 16 palette groups |
| Memory milestone | ✅ | 23/02/2026 | Binary from 49KB to 7KB, 40KB free, external tileset |
| Phase 3 — Interactive gameplay | ✅ | 25/02/2026 | Movement, animation, eating, tunnel, smooth emulator performance |
| Phase 4 — Ghost AI base | ✅ | 02/03/2026 | Scatter/chase, pathfinding, reversal, PM collision |
| Phase 5 — Complete ghost AI | ✅ | 02/03/2026 | Ghost house, frightened, eyes, score chain |
| Phase 6 — Frightened Mode | ✅ | 04/03/2026 | Ghost eating 200-1600, EYES→ENTERHOUSE, flash, level completion |
| Phase 7 — Audio | ✅ | 05/03/2026 | Siren, waka-waka, fright siren, ghost eaten, death jingle |
| Phase 8 — Polish and Levels | ✅ | 07/03/2026 | Lives, death, fruit, Cruise Elroy, scatter/chase per level |
| Phase 9 — Title Screen & Attract Mode | ✅ | 08/03/2026 | Full state machine, attract loop, coin/credit, high score |
| Phase 10 — Intermission & Level 256 | ✅ | 10/03/2026 | 3-act cutscenes, Level 256 bug, intermission music, calibration |
| Phase 11 — Hardware Port & Calibration | ✅ | 14/03/2026 | Running on real hardware, 75Hz calibration, SNES controller, dual-path tileset |
| Phase 11b — Timing Audit & Size Optimization | ✅ | 14/03/2026 | Speed tables corrected, Elroy/dot limits per-level, binary 47014→45928 bytes |

### 22.2 Technical Debt

- **Timing**: arcade-perfect at 3.76s vs 3.78s MAME reference (−0.5%) on real hardware at 75Hz.
- **Binary size**: 45,928 bytes on ~48KB available (~3.2 KB margin). Sufficient for minor features; major additions would require `--max-allocs-per-node 50000` or further structural compression.
### 22.3 Open Items

| Item | Status |
|---|---|
| NOR flash deployment (39SF040 + TL866) | ⏳ Planned |

---

## 23. Architectural Patterns Adopted

The architectural patterns were not chosen arbitrarily: each is the result of a joint evaluation where Azuech defined the priorities (authenticity, simplicity, testability) and Claude proposed technical options with their trade-offs.

### 23.1 From floooh/pacman.c: Temporal Triggers (Decision: Adopt with Reservations)

The most elegant pattern from the reference clone. Instead of complex state machines, every event is a timestamp to compare against the current tick:

```c
typedef struct { uint32_t tick; } trigger_t;

// start(&timer);         → trigger = current tick + 1
// start_after(&timer, n) → trigger = n ticks from now
// now(timer)             → is it exactly this tick?
// since(timer)           → how many ticks have passed?
// after(timer, n)        → have at least n ticks passed?
```

Cost: 4 bytes per timer. Replaces callbacks, event queues, and complex state machines.

### 23.2 From Z-Fighter: Game State Separation

The game follows a state machine with consistent functions for each state:
`state_xxx_enter()`, `state_xxx_update()`, `state_xxx_exit()`.

### 23.3 From snake.c: VBlank Sync and Input

The basic Zeal game loop pattern comes from the Snake game included with the SDK:
`gfx_wait_vblank()` for synchronization, `ioctl()` with `KB_READ_NON_BLOCK` for non-blocking input.

### 23.4 Single Global State

Following the floooh model, all game state lives in a single nested global struct — zero dynamic allocations, maximum predictability on Z80.

---

## 24. Glossary

| Term | Meaning in the ZPac Context |
|---|---|
| **4bpp** | 4 bits per pixel — 16 colors per tile, organized in palette groups |
| **Cornering** | ZPac's ability to start turning before reaching the intersection center |
| **Ghost house** | The rectangular area at the center of the maze where ghosts reside |
| **HostFS** | Zeal emulator feature that mounts a host directory as a virtual drive |
| **Layer0/Layer1** | In Mode 6: layer0 = tilemap (indices), layer1 = attributes (palette, flip) |
| **Logic map** | 28×31 array with bit flags for collisions and gameplay properties |
| **Mode 6** | Zeal video mode: 640×480, 4bpp, 512 tiles, 16 palette groups |
| **No-up zone** | The 2 cells above the ghost house where ghosts cannot turn upward |
| **Palette group** | Group of 16 colors assignable per tile — allows recoloring without duplicating tiles |
| **PSG** | Programmable Sound Generator — the audio chip on the Zeal Video Board |
| **Speed accumulator** | 32-bit pattern rotated left: bit 1 = move, bit 0 = stay |
| **2×2 composite sprite** | 4 hardware 16×16 sprites arranged in a grid to form a 32×32 character |
| **Visual map** | 21×24 array of tile indices for Zeal tilemap rendering |
| **VRAM** | Video RAM (64 KB, separate from Z80 RAM) — where tileset and tilemap reside |
| **WSG** | Waveform Sound Generator — the audio chip of the original arcade version |

---

*Living document — updated at every significant milestone.*
*Last update: Phase 11b complete (March 14, 2026).*
