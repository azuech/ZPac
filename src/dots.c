#include <zvb_gfx.h>
#include "dots.h"
#include "ghost.h"
#include "zpac_types.h"
#include "maze_logic.h"
#include "zpac_maze_data.h"

extern gfx_context vctx;
extern zpac_t zpac;
extern uint16_t tile_nodot_map[];

uint32_t score;

/* Deferred tile replacement queue */
#define DEFER_MAX    8
#define DEFER_FRAMES 3

static struct {
    uint8_t vis_x;
    uint8_t vis_y;
    uint8_t timer;
    uint8_t is_energizer;
} deferred[DEFER_MAX];
static uint8_t defer_count = 0;

static void replace_tile_nodot(uint8_t vis_x, uint8_t vis_y) {
    uint8_t scr_x = MAZE_OFFSET_X + vis_x;
    uint8_t scr_y = MAZE_OFFSET_Y + vis_y;
    uint8_t orig_tile = zpac_maze_map[vis_y][vis_x];
    uint16_t replacement = (orig_tile < 177) ? tile_nodot_map[orig_tile] : TILE_BLANK;
    uint8_t attr = (PAL_MAZE << 4);
    if (replacement >= 256) attr |= 0x01;
    gfx_tilemap_place(&vctx, (uint8_t)(replacement & 0xFF), 0, scr_x, scr_y);
    gfx_tilemap_place(&vctx, attr, 1, scr_x, scr_y);
}

static void defer_tile_replace(uint8_t vis_x, uint8_t vis_y, uint8_t is_energizer) {
    uint8_t i;
    for (i = 0; i < defer_count; i++) {
        if (deferred[i].vis_x == vis_x && deferred[i].vis_y == vis_y) {
            deferred[i].timer = DEFER_FRAMES;
            if (is_energizer) deferred[i].is_energizer = 1;
            return;
        }
    }
    if (defer_count < DEFER_MAX) {
        deferred[defer_count].vis_x = vis_x;
        deferred[defer_count].vis_y = vis_y;
        deferred[defer_count].timer = DEFER_FRAMES;
        deferred[defer_count].is_energizer = is_energizer;
        defer_count++;
    }
}

void dots_init(void) {
    score = 0;
    defer_count = 0;
}

void dots_level_reset(void) {
    defer_count = 0;
}

uint8_t dots_check_eat(void) {
    uint8_t cell = maze_logic[zpac.tile_y][zpac.tile_x];

    if (cell & CELL_DOT) {
        uint8_t vis_x, vis_y;

        maze_logic[zpac.tile_y][zpac.tile_x] &= (uint8_t)~CELL_DOT;
        score += 10;
        dot_count--;
        ghost_house_dot_eaten();

        vis_x = (uint8_t)((zpac.tile_x * 12 + 6) >> 4);
        vis_y = (uint8_t)((zpac.tile_y * 12 + 6) >> 4);
        defer_tile_replace(vis_x, vis_y, 0);

        return 1;
    }

    if (cell & CELL_ENERGIZER) {
        uint8_t vis_x, vis_y;

        maze_logic[zpac.tile_y][zpac.tile_x] &= (uint8_t)~CELL_ENERGIZER;
        score += 50;
        zpac.eat_pause = 3;
        dot_count--;
        ghost_house_dot_eaten();

        vis_x = (uint8_t)((zpac.tile_x * 12 + 6) >> 4);
        vis_y = (uint8_t)((zpac.tile_y * 12 + 6) >> 4);
        defer_tile_replace(vis_x, vis_y, 1);

        return 2;
    }

    return 0;
}

void dots_update(void) {
    uint8_t i = 0;
    while (i < defer_count) {
        if (deferred[i].timer > 0) {
            deferred[i].timer--;
            i++;
        } else {
            uint8_t vx = deferred[i].vis_x;
            uint8_t vy = deferred[i].vis_y;

            replace_tile_nodot(vx, vy);

            if (deferred[i].is_energizer) {
                static const int8_t adj_dx[4] = { 0, 0, -1, 1 };
                static const int8_t adj_dy[4] = { -1, 1, 0, 0 };
                uint8_t a;
                for (a = 0; a < 4; a++) {
                    int8_t ax = (int8_t)vx + adj_dx[a];
                    int8_t ay = (int8_t)vy + adj_dy[a];
                    uint8_t adj_orig;
                    uint16_t adj_repl;
                    if (ax < 0 || ax >= MAZE_COLS || ay < 0 || ay >= MAZE_ROWS) continue;
                    adj_orig = zpac_maze_map[ay][ax];
                    if (adj_orig >= 177) continue;
                    adj_repl = tile_nodot_map[adj_orig];
                    if (adj_repl == adj_orig) continue;
                    replace_tile_nodot((uint8_t)ax, (uint8_t)ay);
                }
            }

            deferred[i] = deferred[defer_count - 1];
            defer_count--;
        }
    }
}

/* Powers of 10 for digit extraction via subtraction (avoids expensive 32-bit division) */
static const uint32_t pow10[7] = {
    1000000UL, 100000UL, 10000UL, 1000UL, 100UL, 10UL, 1UL
};

void score_render(void) {
    uint32_t val = score;
    uint8_t col = 8;  /* leftmost digit position (7 digits: cols 8-14) */
    uint8_t started = 0;
    uint8_t i;

    for (i = 0; i < 7; i++) {
        uint8_t digit = 0;
        while (val >= pow10[i]) {
            val -= pow10[i];
            digit++;
        }
        if (digit > 0 || started || i >= 5) {
            /* Show digit (always show at least last 2 digits: "00") */
            uint16_t tile_idx = FONT_0 + digit;
            uint8_t attr = (uint8_t)(PAL_FONT << 4);
            if (tile_idx >= 256) attr |= 0x01;
            gfx_tilemap_place(&vctx, (uint8_t)(tile_idx & 0xFF), 0, col, 2);
            gfx_tilemap_place(&vctx, attr, 1, col, 2);
            started = 1;
        } else {
            gfx_tilemap_place(&vctx, (uint8_t)(TILE_BLANK & 0xFF), 0, col, 2);
            gfx_tilemap_place(&vctx, TILE_BLANK_ATTR, 1, col, 2);
        }
        col++;
    }
}
