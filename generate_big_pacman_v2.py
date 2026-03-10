#!/usr/bin/env python3
"""
generate_big_pacman_v2.py — Big Pac-Man with smooth mathematical curves.
Draws Pac-Man as a filled circle with mouth at 48x48 native resolution.
"""
import sys
import os
import shutil
import math

TILE_BYTES = 128
SIZE = 48
CENTER_X = 23.5
CENTER_Y = 23.5
RADIUS = 22.0
PAC_COLOR = 3  # yellow in PAL_ZPAC

MOUTH_ANGLES = {
    'wide':   42,
    'med':    22,
    'closed': 0,
}

def draw_pacman_frame(mouth_half_angle_deg):
    pixels = [[0] * SIZE for _ in range(SIZE)]
    mouth_rad = math.radians(mouth_half_angle_deg)
    for y in range(SIZE):
        for x in range(SIZE):
            dx = (x + 0.5) - CENTER_X
            dy = (y + 0.5) - CENTER_Y
            dist = math.sqrt(dx * dx + dy * dy)
            if dist > RADIUS:
                continue
            if mouth_half_angle_deg > 0:
                angle = math.atan2(-dy, dx)
                if abs(angle) < mouth_rad:
                    if dist > 3.0:
                        continue
            pixels[y][x] = PAC_COLOR
    return pixels

def encode_tile_4bpp(pixels):
    data = bytearray(128)
    for row in range(16):
        for col_byte in range(8):
            hi = pixels[row][col_byte * 2] & 0x0F
            lo = pixels[row][col_byte * 2 + 1] & 0x0F
            data[row * 8 + col_byte] = (hi << 4) | lo
    return bytes(data)

def split_into_tiles(pixels, grid_w, grid_h):
    tiles = []
    for gr in range(grid_h):
        for gc in range(grid_w):
            tile = [[0] * 16 for _ in range(16)]
            for r in range(16):
                for c in range(16):
                    pr = gr * 16 + r
                    pc = gc * 16 + c
                    if pr < len(pixels) and pc < len(pixels[0]):
                        tile[r][c] = pixels[pr][pc]
            tiles.append(tile)
    return tiles

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 generate_big_pacman_v2.py <tileset_path>")
        sys.exit(1)

    tileset_path = sys.argv[1]
    with open(tileset_path, 'rb') as f:
        tileset_data = bytearray(f.read())

    current_tiles = len(tileset_data) // TILE_BYTES
    print(f"Current tileset: {len(tileset_data)} bytes, {current_tiles} tiles")

    new_tiles_data = bytearray()
    new_tile_idx = current_tiles
    frame_defines = {}

    for frame_name, mouth_angle in MOUTH_ANGLES.items():
        print(f"\nGenerating frame '{frame_name}' (mouth {mouth_angle} deg):")
        pixels = draw_pacman_frame(mouth_angle)
        tiles_3x3 = split_into_tiles(pixels, 3, 3)
        tile_indices = []
        for tile_pixels in tiles_3x3:
            encoded = encode_tile_4bpp(tile_pixels)
            new_tiles_data += encoded
            tile_indices.append(new_tile_idx)
            new_tile_idx += 1
        frame_defines[frame_name] = tile_indices
        print(f"  Tiles: {tile_indices}")

    tileset_data += new_tiles_data
    with open(tileset_path, 'wb') as f:
        f.write(tileset_data)

    total_new = new_tile_idx - current_tiles
    print(f"\n=== Summary ===")
    print(f"Added {total_new} tiles (indices {current_tiles}-{new_tile_idx - 1})")
    print(f"New tileset: {len(tileset_data)} bytes, {new_tile_idx} tiles")
    print(f"Budget: {new_tile_idx}/512 ({512 - new_tile_idx} remaining)")

    print(f"\n=== #defines for zpac_maze_data.h ===\n")
    print(f"#define TOTAL_TILES       {new_tile_idx}")
    print(f"#define TILESET_SIZE      {new_tile_idx * TILE_BYTES}UL")
    print()
    for frame_name, indices in frame_defines.items():
        name = frame_name.upper()
        labels = ['TL', 'TC', 'TR', 'ML', 'MC', 'MR', 'BL', 'BC', 'BR']
        for i, label in enumerate(labels):
            print(f"#define SPR_BIGPAC_{name}_{label}  {indices[i]}")
        print()

if __name__ == '__main__':
    main()
