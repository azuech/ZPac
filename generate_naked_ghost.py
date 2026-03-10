#!/usr/bin/env python3
"""generate_naked_ghost.py — Naked ghost tiles for Act 3."""
import sys, math

TILE_BYTES = 128
SIZE = 32
PINK, RED, EYE_W, EYE_P = 3, 4, 1, 2

def draw_naked_ghost(frame):
    px = [[0]*SIZE for _ in range(SIZE)]
    base_y, head_cx, head_cy, head_r = 14, 20.0, 19.0, 6.0
    wave = 1.0 if frame == 1 else 0.0
    for y in range(SIZE):
        for x in range(SIZE):
            rx, ry = x+0.5, y+0.5
            rt, rb, rl, rr = base_y+6, base_y+13+wave*0.5, 3.0+wave, 20.0
            if ry >= rt and ry <= rb and rx >= rl and rx <= rr:
                lx = (rx-rl)/(rr-rl)
                if ry >= rt+(1.0-lx)*3.5: px[y][x] = RED
    for y in range(SIZE):
        for x in range(SIZE):
            dx, dy = (x+0.5)-head_cx, (y+0.5)-(head_cy+wave*0.3)
            if math.sqrt(dx*dx+dy*dy) <= head_r: px[y][x] = PINK
    for y in range(SIZE):
        for x in range(SIZE):
            ndx = ((x+0.5)-15.0)/8.0
            ndy = ((y+0.5)-(base_y+5.5+wave*0.2))/4.5
            if (ndx*ndx+ndy*ndy) <= 1.0 and px[y][x] != RED: px[y][x] = PINK
    ey = head_cy-1.5+wave*0.3
    for ex in [head_cx-3.0, head_cx+2.5]:
        for y in range(SIZE):
            for x in range(SIZE):
                d = math.sqrt(((x+0.5)-ex)**2+((y+0.5)-ey)**2)
                if d <= 2.5: px[y][x] = EYE_W
                if d <= 1.2: px[y][x] = EYE_P
    return px

def encode_tile(px):
    d = bytearray(128)
    for r in range(16):
        for c in range(8):
            d[r*8+c] = ((px[r][c*2]&0xF)<<4)|(px[r][c*2+1]&0xF)
    return bytes(d)

def main():
    tp = sys.argv[1]
    with open(tp,'rb') as f: td = bytearray(f.read())
    ct = len(td)//TILE_BYTES
    print(f"Current: {ct} tiles, {len(td)} bytes")
    if ct != 384:
        print(f"ERROR: expected 384, got {ct}. Aborting."); sys.exit(1)
    nd, ni, fi = bytearray(), ct, {}
    for fr in range(2):
        px = draw_naked_ghost(fr)
        idx = []
        for gr in range(2):
            for gc in range(2):
                t = [[px[gr*16+r][gc*16+c] for c in range(16)] for r in range(16)]
                nd += encode_tile(t); idx.append(ni); ni += 1
        fi[fr] = idx; print(f"Frame {fr}: {idx}")
    td += nd
    with open(tp,'wb') as f: f.write(td)
    print(f"\nNew: {ni} tiles, {len(td)} bytes")
    print(f"\n=== For zpac_maze_data.h ===\n")
    print(f"#define TOTAL_TILES       {ni}")
    print(f"#define TILESET_SIZE      {len(td)}UL\n")
    for fr, idx in fi.items():
        for i, l in enumerate(['TL','TR','BL','BR']):
            print(f"#define SPR_NAKED_GHOST_F{fr}_{l}  {idx[i]}")
        print()

if __name__ == '__main__': main()
