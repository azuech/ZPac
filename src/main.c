/**
 * ZPac Phase 2 Test - Mode 6 (640x480, 4bpp)
 * Tileset loaded from "zpac_tiles.bin" (local) or "H:/zpac_tiles.bin" (hostfs).
 * Binary ~7KB, tileset streamed to VRAM in 512-byte chunks.
 * ~40KB free for gameplay code.
 */

#include <zos_sys.h>
#include <zos_vfs.h>
#include <zos_keyboard.h>
#include <zvb_gfx.h>
#include <stdint.h>
#include <string.h>

#include "zpac_maze_data.h"
#include "zpac_types.h"
#include "maze_logic.h"
#include "input.h"
#include "game_loop.h"
#include "dots.h"
#include "ghost.h"
#include "sound.h"

gfx_context vctx;
zpac_t zpac;
uint16_t tile_nodot_map[178];

#define MAZE_PX_X   (MAZE_OFFSET_X * 16)
#define MAZE_PX_Y   (MAZE_OFFSET_Y * 16)
#define ORIG_TO_SPR_X(col) (MAZE_PX_X + (col) * 12 + 7)
#define ORIG_TO_SPR_Y(row) (MAZE_PX_Y + (row) * 12 + 7)

#define HW_SPR_ZPAC   0
#define HW_SPR_BLINKY   4
#define HW_SPR_PINKY    8
#define HW_SPR_INKY     12
#define HW_SPR_CLYDE    16

#define SPRITE_PALETTE(g) ((g) << 4)
#define SPRITE_TILE_BIT9  0x01

/**
 * Check if a 4bpp tile (128 bytes) contains any dot pixels (nibble == 1).
 */
static uint8_t tile_has_dots(const uint8_t *data) {
    uint8_t i;
    for (i = 0; i < 128; i++) {
        if ((data[i] & 0xF0) == 0x10) return 1;
        if ((data[i] & 0x0F) == 0x01) return 1;
    }
    return 0;
}

/**
 * Replace all dot pixels (nibble == 1) with black (0) in a 4bpp tile.
 */
static void tile_remove_dots(uint8_t *data) {
    uint8_t i;
    for (i = 0; i < 128; i++) {
        if ((data[i] & 0xF0) == 0x10) data[i] &= 0x0F;
        if ((data[i] & 0x0F) == 0x01) data[i] &= 0xF0;
    }
}

/**
 * Load tileset from external file into VRAM.
 * Pass 1: load full tileset (tiles 0-340) to VRAM.
 * Pass 2: re-read maze tiles 0-176, generate no-dot variants at tile 341+.
 * Returns 0 on success, 1 if open fails, 2 if read incomplete, 3 if pass 2 fails.
 */
#define CHUNK_SIZE 512
#define TILE_BYTES 128

static const char tileset_path_local[] = "zpac_tiles.bin";
static const char tileset_path_hostfs[] = "H:/zpac_tiles.bin";

static zos_dev_t open_tileset(void) {
    zos_dev_t fd;
    fd = open(tileset_path_local, O_RDONLY);
    if (fd >= 0) return fd;
    return open(tileset_path_hostfs, O_RDONLY);
}

static uint8_t load_tileset_from_file(void) {
    uint8_t buf[CHUNK_SIZE];
    zos_dev_t fd;
    uint16_t offset;
    gfx_tileset_options opts;
    uint16_t tile_idx;
    uint16_t next_free;

    /* --- Pass 1: load entire tileset to VRAM --- */
    fd = open_tileset();
    if (fd < 0) return 1;

    opts.compression = TILESET_COMP_NONE;
    opts.pal_offset = 0;
    opts.opacity = 0;

    offset = 0;
    while (offset < TILESET_SIZE) {
        uint16_t to_read = CHUNK_SIZE;
        if ((uint32_t)offset + to_read > TILESET_SIZE)
            to_read = (uint16_t)(TILESET_SIZE - offset);

        zos_err_t err = read(fd, buf, &to_read);
        if (err != ERR_SUCCESS || to_read == 0) break;

        opts.from_byte = offset;
        gfx_tileset_load(&vctx, buf, to_read, &opts);
        offset += to_read;
    }
    close(fd);
    if (offset < TILESET_SIZE) return 2;

    /* --- Pass 2: generate no-dot tile variants for maze tiles 0-176 --- */
    fd = open_tileset();
    if (fd < 0) return 3;

    next_free = TOTAL_TILES;  /* 341 */
    tile_idx = 0;

    while (tile_idx < 178) {
        uint16_t to_read = CHUNK_SIZE;
        uint16_t pos;
        zos_err_t err = read(fd, buf, &to_read);
        if (err != ERR_SUCCESS || to_read == 0) break;

        pos = 0;
        while (pos + TILE_BYTES <= to_read && tile_idx < 178) {
            uint8_t *tile_data = &buf[pos];
            if (tile_idx == TILE_BLANK || !tile_has_dots(tile_data)) {
                tile_nodot_map[tile_idx] = tile_idx;
            } else {
                tile_remove_dots(tile_data);
                opts.from_byte = (uint32_t)next_free * TILE_BYTES;
                gfx_tileset_load(&vctx, tile_data, TILE_BYTES, &opts);
                tile_nodot_map[tile_idx] = next_free;
                next_free++;
            }
            pos += TILE_BYTES;
            tile_idx++;
        }
    }
    close(fd);

    /* Tiles without dots map to themselves; TILE_BLANK maps to itself */
    tile_nodot_map[TILE_BLANK] = TILE_BLANK;

    return 0;
}

void clean_tilemap(void) {
    uint8_t blank_l0[SCREEN_COLS];
    uint8_t blank_l1[SCREEN_COLS];
    uint8_t y;
    memset(blank_l0, (uint8_t)(TILE_BLANK & 0xFF), SCREEN_COLS);
    memset(blank_l1, TILE_BLANK_ATTR, SCREEN_COLS);
    for (y = 0; y < SCREEN_ROWS; y++) {
        gfx_tilemap_load(&vctx, blank_l0, SCREEN_COLS, 0, 0, y);
        gfx_tilemap_load(&vctx, blank_l1, SCREEN_COLS, 1, 0, y);
    }
}

void put_char(uint8_t col, uint8_t row, uint16_t tile_idx, uint8_t pal) {
    uint8_t attr = (pal << 4);
    if (tile_idx >= 256) attr |= 0x01;
    gfx_tilemap_place(&vctx, tile_idx & 0xFF, 0, col, row);
    gfx_tilemap_place(&vctx, attr, 1, col, row);
}

void put_text(uint8_t col, uint8_t row, const char* text, uint8_t pal) {
    while (*text) {
        char c = *text++;
        uint16_t tile;
        if (c >= '0' && c <= '9')      tile = FONT_0 + (c - '0');
        else if (c >= 'A' && c <= 'Z')  tile = FONT_A + (c - 'A');
        else if (c == ' ')              tile = TILE_BLANK;
        else if (c == '!')              tile = FONT_EXCL;
        else if (c == '-')              tile = FONT_DASH;
        else if (c == '/')              tile = FONT_SLASH;
        else if (c == '.')              tile = FONT_DOT;
        else if (c == '"')              tile = FONT_DQUOTE;
        else { col++; continue; }
        put_char(col++, row, tile, pal);
    }
}

void render_hud(void) {
    put_text(11, 1, "1UP", PAL_FONT);
    put_text(15, 1, "HIGH SCORE", PAL_FONT);
}

int main(void) {
    gfx_enable_screen(0);
    gfx_error err = gfx_initialize(ZVB_CTRL_VID_MODE_GFX_640_4BIT, &vctx);
    if (err) return 1;

    gfx_tileset_add_color_tile(&vctx, TILE_BLANK, 0);
    clean_tilemap();
    gfx_palette_load(&vctx, (void*)zpac_palette, 512, 0);

    /* Stream tileset from zpac_tiles.bin (local or H:/) into VRAM */
    uint8_t load_err = load_tileset_from_file();
    if (load_err) {
        /* Show error on screen for debugging */
        gfx_enable_screen(1);
        return 10 + load_err;  /* 11 = open failed, 12 = read incomplete */
    }

    gfx_tileset_add_color_tile(&vctx, TILE_BLANK, 0);
    gfx_enable_screen(1);

    sound_init();
    input_init();
    game_loop_run();  /* Never returns */

    return 0;
}
