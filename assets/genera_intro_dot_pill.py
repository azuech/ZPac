#!/usr/bin/env python3
"""
Generate dot and pill (energizer) intro tiles from zpac ROM data.
Tiles 0x10 (dot) and 0x14 (pill) decoded,
scaled 2x to 16x16, converted to 4bpp, and appended to zpac_tileset.bin.
"""

import re
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
REFERENCE_FILE = os.path.join(PROJECT_DIR, "Floooh reference", "pacman.c")
TILESET_FILE = os.path.join(PROJECT_DIR, "build", "zpac_tileset.bin")

TILE_DOT  = 0x10  # small dot
TILE_PILL = 0x14  # energizer


def parse_rom_tiles(filepath):
    with open(filepath, 'r') as f:
        src = f.read()
    match = re.search(r'static\s+const\s+uint8_t\s+rom_tiles\[4096\]\s*=\s*\{([^}]+)\}', src)
    if not match:
        print("ERROR: rom_tiles[4096] not found")
        sys.exit(1)
    values = re.findall(r'0x[0-9a-fA-F]+|\d+', match.group(1))
    return [int(v, 0) for v in values]


def decode_tile_8x4(rom_tiles, tile_code, stride, offset):
    pixels = [[0]*8 for _ in range(4)]
    for tx in range(8):
        ti = tile_code * stride + offset + (7 - tx)
        for ty in range(4):
            p_hi = (rom_tiles[ti] >> (7 - ty)) & 1
            p_lo = (rom_tiles[ti] >> (3 - ty)) & 1
            pixels[ty][tx] = (p_hi << 1) | p_lo
    return pixels


def decode_tile_8x8(rom_tiles, tile_code):
    top = decode_tile_8x4(rom_tiles, tile_code, 16, 8)
    bottom = decode_tile_8x4(rom_tiles, tile_code, 16, 0)
    return top + bottom


def scale_2x(tile_8x8):
    out = []
    for row in tile_8x8:
        scaled = []
        for px in row:
            scaled.extend([px, px])
        out.append(scaled)
        out.append(list(scaled))
    return out


def tile_to_4bpp(tile_16x16):
    data = bytearray()
    for row in tile_16x16:
        for i in range(0, 16, 2):
            data.append((row[i] << 4) | (row[i+1] & 0x0F))
    assert len(data) == 128
    return data


def print_ascii(tile, label):
    chars = ['.', '#', '@', 'O']
    print(f"  {label}:")
    for row in tile:
        print('  ' + ''.join(chars[px & 3] for px in row))


def main():
    print("=== Dot & Pill Intro Tile Generator ===")

    rom_tiles = parse_rom_tiles(REFERENCE_FILE)
    print(f"Parsed {len(rom_tiles)} bytes from rom_tiles")

    all_data = bytearray()

    for tc, name in [(TILE_DOT, "DOT (0x10)"), (TILE_PILL, "PILL (0x14)")]:
        t8 = decode_tile_8x8(rom_tiles, tc)
        print_ascii(t8, f"{name} 8x8")
        t16 = scale_2x(t8)
        print_ascii(t16, f"{name} 16x16 (2x scaled)")
        all_data.extend(tile_to_4bpp(t16))

    print(f"\nGenerated {len(all_data)} bytes ({len(all_data)//128} tiles)")

    if not os.path.exists(TILESET_FILE):
        print(f"ERROR: {TILESET_FILE} not found")
        sys.exit(1)

    old_size = os.path.getsize(TILESET_FILE)
    old_tiles = old_size // 128
    print(f"Current tileset: {old_size} bytes ({old_tiles} tiles)")

    with open(TILESET_FILE, 'ab') as f:
        f.write(all_data)

    new_size = os.path.getsize(TILESET_FILE)
    new_tiles = new_size // 128
    print(f"New tileset:     {new_size} bytes ({new_tiles} tiles)")
    print(f"\nNew tile indices:")
    print(f"  TILE_INTRO_DOT  = {old_tiles}")
    print(f"  TILE_INTRO_PILL = {old_tiles + 1}")
    print("Done!")


if __name__ == '__main__':
    main()
