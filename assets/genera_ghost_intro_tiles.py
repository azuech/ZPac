#!/usr/bin/env python3
"""
Generate 6 ghost intro tiles (2x3 grid, scaled 2x to 16x16 each)
from the zpac ROM tile data.

Tiles 0xB0-0xB5 decoded using gfx_decode_tile method,
scaled 2x (nearest neighbor), converted to 4bpp, and appended
to zpac_tileset.bin.
"""

import re
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
REFERENCE_FILE = os.path.join(PROJECT_DIR, "Floooh reference", "pacman.c")
TILESET_FILE = os.path.join(PROJECT_DIR, "build", "zpac_tileset.bin")

# Tile codes for the ghost intro (0xB0-0xB5 = 176-181)
GHOST_INTRO_TILES = [0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5]


def parse_rom_tiles(filepath):
    """Parse the rom_tiles[4096] array from the reference C source."""
    with open(filepath, 'r') as f:
        src = f.read()

    # Find the rom_tiles definition
    match = re.search(r'static\s+const\s+uint8_t\s+rom_tiles\[4096\]\s*=\s*\{([^}]+)\}', src)
    if not match:
        print("ERROR: rom_tiles[4096] not found in source file")
        sys.exit(1)

    data_str = match.group(1)
    # Extract all hex/decimal values
    values = re.findall(r'0x[0-9a-fA-F]+|\d+', data_str)
    rom = [int(v, 0) for v in values]

    if len(rom) != 4096:
        print(f"WARNING: Expected 4096 bytes, got {len(rom)}")

    return rom


def decode_tile_8x4(rom_tiles, tile_code, stride, offset):
    """Decode half a tile (8x4 pixels).
    Returns a 4x8 array (rows x cols)."""
    pixels = [[0]*8 for _ in range(4)]
    for tx in range(8):
        ti = tile_code * stride + offset + (7 - tx)
        for ty in range(4):
            p_hi = (rom_tiles[ti] >> (7 - ty)) & 1
            p_lo = (rom_tiles[ti] >> (3 - ty)) & 1
            p = (p_hi << 1) | p_lo
            pixels[ty][tx] = p
    return pixels


def decode_tile_8x8(rom_tiles, tile_code):
    """Decode a full 8x8 tile. Top half uses offset=8, bottom half offset=0."""
    top = decode_tile_8x4(rom_tiles, tile_code, 16, 8)
    bottom = decode_tile_8x4(rom_tiles, tile_code, 16, 0)
    return top + bottom  # 8 rows x 8 cols


def scale_2x(tile_8x8):
    """Scale 8x8 tile to 16x16 using nearest neighbor."""
    out = []
    for row in tile_8x8:
        scaled_row = []
        for px in row:
            scaled_row.extend([px, px])
        out.append(scaled_row)
        out.append(list(scaled_row))  # duplicate row
    return out


def tile_to_4bpp(tile_16x16):
    """Convert 16x16 pixel array to 4bpp binary (128 bytes).
    High nibble = left pixel, low nibble = right pixel."""
    data = bytearray()
    for row in tile_16x16:
        for i in range(0, 16, 2):
            byte = (row[i] << 4) | (row[i+1] & 0x0F)
            data.append(byte)
    assert len(data) == 128
    return data


def print_ascii_tile(tile, label=""):
    """Print a tile as ASCII art."""
    chars = ['.', '#', '@', 'O']
    if label:
        print(f"  {label}:")
    for row in tile:
        line = '  '
        for px in row:
            line += chars[px & 3]
        print(line)


def print_ghost_composite(tiles_8x8):
    """Print the full 2x3 ghost as ASCII art (16x24 before scaling)."""
    chars = ['.', '#', '@', 'O']
    print("\n=== Ghost Intro (8x8 tiles, before 2x scaling) ===")
    # 3 rows of tiles, 2 tiles per row
    for tr in range(3):
        top = tiles_8x8[tr * 2]      # left tile
        right = tiles_8x8[tr * 2 + 1]  # right tile
        for row in range(8):
            line = '  '
            for px in top[row]:
                line += chars[px & 3]
            for px in right[row]:
                line += chars[px & 3]
            print(line)


def main():
    print("=== Ghost Intro Tile Generator ===")
    print(f"Reference: {REFERENCE_FILE}")
    print(f"Tileset:   {TILESET_FILE}")

    # Parse ROM data
    rom_tiles = parse_rom_tiles(REFERENCE_FILE)
    print(f"Parsed {len(rom_tiles)} bytes from rom_tiles")

    # Decode the 6 tiles
    tiles_8x8 = []
    for tc in GHOST_INTRO_TILES:
        t = decode_tile_8x8(rom_tiles, tc)
        tiles_8x8.append(t)
        print_ascii_tile(t, f"Tile 0x{tc:02X} ({tc})")

    # Print composite ghost
    print_ghost_composite(tiles_8x8)

    # Scale 2x and convert to 4bpp
    all_data = bytearray()
    tiles_16x16 = []
    for i, t8 in enumerate(tiles_8x8):
        t16 = scale_2x(t8)
        tiles_16x16.append(t16)
        data = tile_to_4bpp(t16)
        all_data.extend(data)

    print(f"\nGenerated {len(all_data)} bytes ({len(all_data)//128} tiles x 128 bytes)")

    # Print scaled composite
    chars = ['.', '#', '@', 'O']
    print("\n=== Ghost Intro (16x16 tiles, after 2x scaling) ===")
    for tr in range(3):
        left = tiles_16x16[tr * 2]
        right = tiles_16x16[tr * 2 + 1]
        for row in range(16):
            line = '  '
            for px in left[row]:
                line += chars[px & 3]
            for px in right[row]:
                line += chars[px & 3]
            print(line)

    # Check current tileset size
    if not os.path.exists(TILESET_FILE):
        print(f"\nERROR: Tileset file not found: {TILESET_FILE}")
        sys.exit(1)

    old_size = os.path.getsize(TILESET_FILE)
    old_tiles = old_size // 128
    print(f"\nCurrent tileset: {old_size} bytes ({old_tiles} tiles)")

    # Append to tileset
    with open(TILESET_FILE, 'ab') as f:
        f.write(all_data)

    new_size = os.path.getsize(TILESET_FILE)
    new_tiles = new_size // 128
    print(f"New tileset:     {new_size} bytes ({new_tiles} tiles)")
    print(f"Added {len(all_data)} bytes ({len(all_data)//128} tiles)")

    # Print assigned indices
    print("\nNew tile indices:")
    names = ["GHOST_INTRO_TL", "GHOST_INTRO_TR",
             "GHOST_INTRO_ML", "GHOST_INTRO_MR",
             "GHOST_INTRO_BL", "GHOST_INTRO_BR"]
    for i, name in enumerate(names):
        idx = old_tiles + i
        print(f"  {name} = {idx}")

    print("\nDone!")


if __name__ == '__main__':
    main()
