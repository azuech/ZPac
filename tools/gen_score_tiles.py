#!/usr/bin/env python3
"""
Generate 8 score popup tiles (2 per value: 200, 400, 800, 1600).
Uses a 5x7 pixel font, centered in a 32x16 area (2 tiles of 16x16).
Pixel value 1 = lit (cyan in PAL_GHOST_SCORE).
Replaces previous 4-tile version.
"""
import os

# 5x7 pixel font - each digit is a list of 7 strings
DIGITS = {
    '0': ['01110',
          '10001',
          '10011',
          '10101',
          '11001',
          '10001',
          '01110'],
    '1': ['00100',
          '01100',
          '00100',
          '00100',
          '00100',
          '00100',
          '01110'],
    '2': ['01110',
          '10001',
          '00001',
          '00010',
          '00100',
          '01000',
          '11111'],
    '3': ['01110',
          '10001',
          '00001',
          '00110',
          '00001',
          '10001',
          '01110'],
    '4': ['00010',
          '00110',
          '01010',
          '10010',
          '11111',
          '00010',
          '00010'],
    '5': ['11111',
          '10000',
          '11110',
          '00001',
          '00001',
          '10001',
          '01110'],
    '6': ['00110',
          '01000',
          '10000',
          '11110',
          '10001',
          '10001',
          '01110'],
    '7': ['11111',
          '00001',
          '00010',
          '00100',
          '01000',
          '01000',
          '01000'],
    '8': ['01110',
          '10001',
          '10001',
          '01110',
          '10001',
          '10001',
          '01110'],
    '9': ['01110',
          '10001',
          '10001',
          '01111',
          '00001',
          '00010',
          '01100'],
}

FONT_W = 5   # digit width
FONT_H = 7   # digit height
GAP = 1      # pixels between digits
CANVAS_W = 32  # 2 tiles wide
CANVAS_H = 16  # 1 tile tall

def render_number(text):
    """Render number string into 32x16 pixel grid (2 tiles)."""
    total_w = len(text) * FONT_W + (len(text) - 1) * GAP
    start_x = (CANVAS_W - total_w) // 2
    start_y = (CANVAS_H - FONT_H) // 2

    grid = [[0] * CANVAS_W for _ in range(CANVAS_H)]
    cx = start_x
    for ch in text:
        pattern = DIGITS[ch]
        for row in range(FONT_H):
            for col in range(FONT_W):
                if pattern[row][col] == '1':
                    grid[start_y + row][cx + col] = 1
        cx += FONT_W + GAP
    return grid

def grid_to_4bpp_pair(grid):
    """Convert 32x16 grid to two 128-byte 4bpp tiles (left + right)."""
    left_tile = bytearray(128)
    right_tile = bytearray(128)
    for y in range(16):
        for xpair in range(8):
            lx = xpair * 2
            l_left = grid[y][lx] & 0x0F
            l_right = grid[y][lx + 1] & 0x0F
            left_tile[y * 8 + xpair] = (l_left << 4) | l_right
            rx = 16 + xpair * 2
            r_left = grid[y][rx] & 0x0F
            r_right = grid[y][rx + 1] & 0x0F
            right_tile[y * 8 + xpair] = (r_left << 4) | r_right
    return left_tile, right_tile

# === Main ===
tileset_path = os.path.expanduser('~/zpac_phase1_test/build/zpac_tileset.bin')

# Truncate back to original 341 tiles
ORIGINAL_TILES = 341
TILE_BYTES = 128
original_size = ORIGINAL_TILES * TILE_BYTES

with open(tileset_path, 'r+b') as f:
    f.truncate(original_size)
print(f"Truncated to {original_size} bytes ({ORIGINAL_TILES} tiles)")

# Generate and append 8 new tiles (2 per score value)
scores = ['200', '400', '800', '1600']
tile_data = bytearray()
for s in scores:
    grid = render_number(s)
    left, right = grid_to_4bpp_pair(grid)
    tile_data.extend(left)
    tile_data.extend(right)

with open(tileset_path, 'ab') as f:
    f.write(tile_data)

new_size = os.path.getsize(tileset_path)
print(f"Appended 8 score tiles ({len(tile_data)} bytes)")
print(f"New file size: {new_size} bytes ({new_size // TILE_BYTES} tiles)")
print(f"Tile indices:")
idx = ORIGINAL_TILES
for s in scores:
    print(f"  {s}: left={idx}, right={idx+1}")
    idx += 2
