#!/usr/bin/env python3
"""
generate_big_pacman.py — Generate 2x scaled Big Pac-Man tiles for cutscenes.
Reads the existing tileset, extracts Pac-Man right-facing frames,
scales 2x content to 48x48, splits in 3x3 grid of 16x16 tiles,
and appends to the tileset binary.
"""
import sys
import os
import shutil

TILE_BYTES = 128

FRAMES = {
    'wide':   {'TL': 178, 'TR': 179, 'BL': 180, 'BR': 181},
    'med':    {'TL': 185, 'TR': 186, 'BL': 187, 'BR': 188},
    'closed': {'TL': 193, 'TR': 190, 'BL': 187, 'BR': 194},
}

def decode_tile_4bpp(data):
    pixels = [[0]*16 for _ in range(16)]
    for row in range(16):
        for col_byte in range(8):
            byte = data[row * 8 + col_byte]
            pixels[row][col_byte * 2]     = (byte >> 4) & 0x0F
            pixels[row][col_byte * 2 + 1] = byte & 0x0F
    return pixels

def encode_tile_4bpp(pixels):
    data = bytearray(128)
    for row in range(16):
        for col_byte in range(8):
            hi = pixels[row][col_byte * 2] & 0x0F
            lo = pixels[row][col_byte * 2 + 1] & 0x0F
            data[row * 8 + col_byte] = (hi << 4) | lo
    return bytes(data)

def extract_tile(tileset_data, tile_idx):
    offset = tile_idx * TILE_BYTES
    raw = tileset_data[offset:offset + TILE_BYTES]
    return decode_tile_4bpp(raw)

def assemble_2x2(tl, tr, bl, br):
    result = [[0]*32 for _ in range(32)]
    for r in range(16):
        for c in range(16):
            result[r][c]      = tl[r][c]
            result[r][c+16]   = tr[r][c]
            result[r+16][c]   = bl[r][c]
            result[r+16][c+16]= br[r][c]
    return result

def scale_2x(pixels, w, h):
    result = [[0]*(w*2) for _ in range(h*2)]
    for r in range(h):
        for c in range(w):
            v = pixels[r][c]
            result[r*2][c*2]     = v
            result[r*2][c*2+1]   = v
            result[r*2+1][c*2]   = v
            result[r*2+1][c*2+1] = v
    return result

def split_into_tiles(pixels, grid_w, grid_h):
    tiles = []
    for gr in range(grid_h):
        for gc in range(grid_w):
            tile = [[0]*16 for _ in range(16)]
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
        print("Usage: python3 generate_big_pacman.py <tileset_path>")
        sys.exit(1)

    tileset_path = sys.argv[1]
    with open(tileset_path, 'rb') as f:
        tileset_data = bytearray(f.read())

    current_tiles = len(tileset_data) // TILE_BYTES
    print(f"Current tileset: {len(tileset_data)} bytes, {current_tiles} tiles")

    backup_path = tileset_path + '.bak'
    if not os.path.exists(backup_path):
        shutil.copy2(tileset_path, backup_path)
        print(f"Backup saved to {backup_path}")

    new_tiles_data = bytearray()
    new_tile_idx = current_tiles
    frame_defines = {}

    for frame_name, indices in FRAMES.items():
        print(f"\nProcessing frame '{frame_name}':")

        tl = extract_tile(tileset_data, indices['TL'])
        tr = extract_tile(tileset_data, indices['TR'])
        bl = extract_tile(tileset_data, indices['BL'])
        br = extract_tile(tileset_data, indices['BR'])

        composite = assemble_2x2(tl, tr, bl, br)

        # Extract 24x24 content area (4px padding on each side of 32x32)
        content = [row[4:28] for row in composite[4:28]]

        big = scale_2x(content, 24, 24)

        tiles_3x3 = split_into_tiles(big, 3, 3)

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
