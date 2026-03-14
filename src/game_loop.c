#include <zos_sys.h>
#include <zos_vfs.h>
#include <zos_keyboard.h>
#include <zvb_gfx.h>
#include <string.h>
#include "game_loop.h"
#include "input.h"
#include "zpac_types.h"
#include "maze_logic.h"
#include "zpac_maze_data.h"
#include "dots.h"
#include "ghost.h"
#include "sound.h"
#include "fruit.h"
#include "level256.h"
#include "cutscene.h"

extern gfx_context vctx;
extern zpac_t zpac;
extern uint16_t tile_nodot_map[];

extern void put_text(uint8_t col, uint8_t row, const char* text, uint8_t pal);
extern void put_char(uint8_t col, uint8_t row, uint16_t tile_idx, uint8_t pal);
extern void clean_tilemap(void);
extern void render_hud(void);

static uint8_t game_state;
static uint8_t credits;
uint32_t high_score;
static uint8_t demo_mode;
static uint16_t demo_frame_count;
static uint8_t pending_coin_sound;
static uint8_t pending_start;
#define DEMO_DURATION  3750

uint8_t game_level = 0;
uint8_t lives = 3;
uint8_t extra_life_given = 0;
speed_set_t cur_spd;

/* 4 speed tiers: L1, L2-4, L5-20, L21+ */
static const speed_set_t speed_tiers[4] = {
    /*  pac_n  pac_f  pac_d  gho_n  gho_f  gho_t */
    {   346,   404,   289,   318,   173,   115 },  /* Tier 0: L1 (75fps) */
    {   390,   433,   318,   361,   187,   130 },  /* Tier 1: L2-4 */
    {   433,   462,   346,   404,   202,   144 },  /* Tier 2: L5-20 */
    {   390,   462,   318,   404,   202,   144 }   /* Tier 3: L21+ */
};

void update_level_speeds(void) {
    uint8_t tier;
    if (game_level == 0)       tier = 0;
    else if (game_level <= 3)  tier = 1;
    else if (game_level <= 19) tier = 2;
    else                       tier = 3;
    cur_spd = speed_tiers[tier];
}

/* Fright duration per level in frames at 75fps */
static const uint16_t fright_dur_table[21] = {
    450, 375, 300, 225, 150, 375, 150, 150,  75, 375,
    150,  75,  75, 225,  75,  75,   0,  75,   0,   0, 0
};

uint16_t get_fright_duration(void) {
    uint8_t idx = game_level;
    if (idx >= 21) idx = 20;
    return (uint16_t)fright_dur_table[idx];
}

/* Check if the adjacent tile in the given direction is walkable */
static uint8_t can_walk(uint8_t tile_x, uint8_t tile_y, uint8_t dir) {
    int8_t nx = (int8_t)tile_x + dir_dx[dir];
    int8_t ny = (int8_t)tile_y + dir_dy[dir];

    if (nx < 0) nx = 27;
    if (nx > 27) nx = 0;
    if (ny < 0 || ny > 30) return 0;

    {
        uint8_t cell = maze_logic[ny][nx];
        if (!(cell & CELL_WALKABLE)) return 0;
        if (cell & CELL_GHOST_HOUSE) return 0;
        return 1;
    }
}

/* Hardware sprite indices for life icons (max 5 icons, 4 sprites each) */
#define HW_SPR_LIFE_BASE  24
#define MAX_LIFE_ICONS    5

/* Calibrated sprite offsets */
#define SPR_OFF_X  1
#define SPR_OFF_Y  1

/* Sprite flip flags */
#ifndef SPRITE_FLIP_X
#define SPRITE_FLIP_X  0x08
#define SPRITE_FLIP_Y  0x04
#endif

/* Tile table per animation frame: [phase][quadrant TL,TR,BL,BR]
 * Phase 0=wide, 1=medium, 2=closed
 * Animation cycle: wide→med→closed→med (map: 0,1,2,1) */

/* For RIGHT (and LEFT via FLIP_X) */
static const uint16_t pac_right_tiles[3][4] = {
    { SPR_PAC_RIGHT_WIDE_TL, SPR_PAC_RIGHT_WIDE_TR,
      SPR_PAC_RIGHT_WIDE_BL, SPR_PAC_RIGHT_WIDE_BR },
    { SPR_PAC_RIGHT_MED_TL, SPR_PAC_RIGHT_MED_TR,
      SPR_PAC_RIGHT_MED_BL, SPR_PAC_RIGHT_MED_BR },
    { SPR_PAC_CLOSED_TL, SPR_PAC_CLOSED_TR,
      SPR_PAC_CLOSED_BL, SPR_PAC_CLOSED_BR }
};

/* For DOWN (and UP via FLIP_Y) */
static const uint16_t pac_up_tiles[3][4] = {
    { SPR_PAC_UP_WIDE_TL, SPR_PAC_UP_WIDE_TR,
      SPR_PAC_UP_WIDE_BL, SPR_PAC_UP_WIDE_BR },
    { SPR_PAC_UP_MED_TL, SPR_PAC_UP_MED_TR,
      SPR_PAC_UP_MED_BL, SPR_PAC_UP_MED_BR },
    { SPR_PAC_CLOSED_TL, SPR_PAC_CLOSED_TR,
      SPR_PAC_CLOSED_BL, SPR_PAC_CLOSED_BR }
};

/* Animation phase map: cycle 0→1→2→1 */
static const uint8_t anim_phase_map[4] = { 0, 1, 2, 1 };

static uint16_t tile_sub_to_pixel(uint8_t tile, uint8_t sub) {
    return ((uint16_t)tile << 3) + ((uint16_t)tile << 2) + sub;
}

static void update_zpac_sprites(void) {
    uint16_t px = tile_sub_to_pixel(zpac.tile_x, zpac.sub_x);
    uint16_t py = tile_sub_to_pixel(zpac.tile_y, zpac.sub_y);
    uint16_t sx = 144 + px + SPR_OFF_X;
    uint16_t sy =  48 + py + SPR_OFF_Y;

    /* Compute animation phase: advances every 2 ticks */
    uint8_t phase = anim_phase_map[(zpac.anim_tick >> 2) & 3];

    /* Select tiles and flags based on direction */
    const uint16_t *tiles;
    uint8_t flip_flags = 0;
    uint8_t flip_x = 0;
    uint8_t flip_y = 0;

    switch (zpac.dir_current) {
        case DIR_RIGHT:
            tiles = pac_right_tiles[phase];
            break;
        case DIR_LEFT:
            tiles = pac_right_tiles[phase];
            flip_x = 1;
            flip_flags = SPRITE_FLIP_X;
            break;
        case DIR_DOWN:
            tiles = pac_up_tiles[phase];
            break;
        case DIR_UP:
            tiles = pac_up_tiles[phase];
            flip_y = 1;
            flip_flags = SPRITE_FLIP_Y;
            break;
        default:
            tiles = pac_right_tiles[phase];
            break;
    }

    /* Offsets for the 4 quadrants */
    uint16_t dx_off[4] = {0, 16, 0, 16};
    uint16_t dy_off[4] = {0, 0, 16, 16};

    /* Quadrant mapping for flip:
     * No flip:    pos0=TL(0), pos1=TR(1), pos2=BL(2), pos3=BR(3)
     * With FLIP_X: pos0=TR(1), pos1=TL(0), pos2=BR(3), pos3=BL(2)
     * With FLIP_Y: pos0=BL(2), pos1=BR(3), pos2=TL(0), pos3=TR(1) */
    uint8_t tile_map[4];
    if (flip_x) {
        tile_map[0] = 1; tile_map[1] = 0;
        tile_map[2] = 3; tile_map[3] = 2;
    } else if (flip_y) {
        tile_map[0] = 2; tile_map[1] = 3;
        tile_map[2] = 0; tile_map[3] = 1;
    } else {
        tile_map[0] = 0; tile_map[1] = 1;
        tile_map[2] = 2; tile_map[3] = 3;
    }

    uint8_t i;
    for (i = 0; i < 4; i++) {
        uint16_t t = tiles[tile_map[i]];
        gfx_sprite spr = { 0 };
        spr.x = sx + dx_off[i];
        spr.y = sy + dy_off[i];
        spr.tile = t & 0xFF;
        spr.flags = (PAL_ZPAC << 4) | flip_flags;
        if (t >= 256) spr.flags |= 0x01;
        gfx_sprite_render(&vctx, i, &spr);
    }
}

/* Re-render the maze tilemap */
static void redraw_maze(void) {
    uint8_t row;
    for (row = 0; row < MAZE_ROWS; row++) {
        uint8_t tile_row_l0[SCREEN_COLS];
        uint8_t tile_row_l1[SCREEN_COLS];
        uint8_t col;
        memset(tile_row_l0, (uint8_t)(TILE_BLANK & 0xFF), SCREEN_COLS);
        memset(tile_row_l1, TILE_BLANK_ATTR, SCREEN_COLS);
        for (col = 0; col < MAZE_COLS; col++) {
            uint8_t sx = MAZE_OFFSET_X + col;
            tile_row_l0[sx] = zpac_maze_map[row][col];
            tile_row_l1[sx] = zpac_maze_attr[row][col];
        }
        gfx_tilemap_load(&vctx, tile_row_l0, SCREEN_COLS, 0, 0, MAZE_OFFSET_Y + row);
        gfx_tilemap_load(&vctx, tile_row_l1, SCREEN_COLS, 1, 0, MAZE_OFFSET_Y + row);
    }
}

/* Hide all ghost sprites off-screen */
static void hide_all_ghosts(void) {
    uint8_t i;
    for (i = 0; i < NUM_GHOSTS * 4; i++) {
        gfx_sprite spr = { 0 };
        spr.x = 0;
        spr.y = 480;
        gfx_sprite_render(&vctx, HW_SPR_GHOST_BASE + i, &spr);
    }
}

/* Restore maze tiles at screen row 16, cols 17-22 (area used by READY! text). */
static void restore_ready_tiles(void) {
    uint8_t ri;
    for (ri = 0; ri < 6; ri++) {
        uint8_t scr_col = 17 + ri;
        uint8_t maze_col = scr_col - MAZE_OFFSET_X;
        uint8_t maze_row = 16 - MAZE_OFFSET_Y;
        uint8_t tile = zpac_maze_map[maze_row][maze_col];
        uint8_t attr = zpac_maze_attr[maze_row][maze_col];
        gfx_tilemap_place(&vctx, tile, 0, scr_col, 16);
        gfx_tilemap_place(&vctx, attr, 1, scr_col, 16);
    }
}

static void lives_render(void);

/* Full level complete sequence: freeze, flash, reset */
static void level_complete_sequence(void) {
    uint8_t f;
    sound_stop_all();

    /* Phase 1: Freeze ~1 sec */
    hide_all_ghosts();
    update_zpac_sprites();
    for (f = 0; f < 31; f++) {
        gfx_wait_end_vblank(&vctx);
        gfx_wait_vblank(&vctx);
    }

    /* Phase 2: Flash maze walls ~3 sec.
     * Swap entire palette group 0 between original (blue walls)
     * and modified (white walls) to guarantee the change takes effect. */
    {
        uint8_t white_pal[32];
        uint8_t fi;
        memcpy(white_pal, zpac_palette, 32);
        /* Set color index 1 (dots/outline) to white */
        white_pal[2] = 0xFF; white_pal[3] = 0xFF;
        /* Set color index 3 (walls) to white */
        white_pal[6] = 0xFF; white_pal[7] = 0xFF;

        for (fi = 0; fi < 8; fi++) {
            uint8_t wi;
            if ((fi & 1) == 0) {
                gfx_palette_load(&vctx, white_pal, 32, 0);
            } else {
                gfx_palette_load(&vctx, (void*)zpac_palette, 32, 0);
            }
            for (wi = 0; wi < 13; wi++) {
                gfx_wait_end_vblank(&vctx);
                gfx_wait_vblank(&vctx);
            }
        }
    }
    /* Restore entire palette to guarantee clean state */
    gfx_palette_load(&vctx, (void*)zpac_palette, 512, 0);

    /* Phase 2b: Check for intermission cutscene */
    {
        /* Peek at what the next level will be */
        uint8_t next_level = game_level + 1;
        if (cutscene_check_and_play(next_level)) {
            /* Cutscene was played — need to restore palette
             * (cutscene uses clean_tilemap which clears screen) */
            gfx_palette_load(&vctx, (void*)zpac_palette, 512, 0);
        }
    }

    /* Phase 3: Increment level and reset */
    game_level++;
    update_level_speeds();

    init_maze_logic();
    redraw_maze();

    /* Level 256 split-screen bug */
    if (game_level == 255) {
        level256_apply();
    }

    dots_level_reset();
    score_render();
    lives_render();
    fruit_init();
    fruit_hud_render();
    ghosts_init();

    zpac.tile_x = 14;
    zpac.tile_y = 23;
    zpac.sub_x = 6;
    zpac.sub_y = 6;
    zpac.dir_current = DIR_LEFT;
    zpac.dir_desired = DIR_LEFT;
    zpac.speed_acc = 0;
    zpac.anim_tick = 0;
    zpac.eat_pause = 0;
    update_zpac_sprites();
    ghosts_render();

    /* Phase 4: "READY!" ~2 sec */
    put_text(17, 16, "READY!", PAL_ZPAC);
    for (f = 0; f < 63; f++) {
        gfx_wait_end_vblank(&vctx);
        gfx_wait_vblank(&vctx);
    }
    restore_ready_tiles();
    sound_start_siren();
}

/* Render life icons as 2x2 composite sprites on row 27 (above CREDIT).
 * Shows (lives - 1) icons (spare lives, not the active one).
 * Each icon is zpac with mouth open facing left (FLIP_X).
 * Row 27 = y pixel 432. Icons spaced 34px apart starting at x=148. */
static void lives_render(void) {
    uint8_t spare;
    uint8_t icon;
    uint16_t py = 432;  /* row 27 * 16 = 432 */

    spare = (lives > 0) ? (lives - 1) : 0;
    if (spare > MAX_LIFE_ICONS) spare = MAX_LIFE_ICONS;

    for (icon = 0; icon < MAX_LIFE_ICONS; icon++) {
        uint8_t base_spr = HW_SPR_LIFE_BASE + (icon << 2);

        if (icon < spare) {
            uint16_t px = 148 + (uint16_t)icon * 16;
            uint16_t tiles[4];
            uint8_t tile_map[4];
            uint16_t dx_off[4];
            uint16_t dy_off[4];
            uint8_t qi;

            /* zpac right wide open tiles (bocca aperta a destra) */
            tiles[0] = SPR_PAC_RIGHT_WIDE_TL;
            tiles[1] = SPR_PAC_RIGHT_WIDE_TR;
            tiles[2] = SPR_PAC_RIGHT_WIDE_BL;
            tiles[3] = SPR_PAC_RIGHT_WIDE_BR;

            dx_off[0] = 0;  dx_off[1] = 16; dx_off[2] = 0;  dx_off[3] = 16;
            dy_off[0] = 0;  dy_off[1] = 0;  dy_off[2] = 16; dy_off[3] = 16;

            /* FLIP_X: swap TL<->TR and BL<->BR tile content */
            tile_map[0] = 1; tile_map[1] = 0;
            tile_map[2] = 3; tile_map[3] = 2;

            for (qi = 0; qi < 4; qi++) {
                uint16_t t = tiles[tile_map[qi]];
                gfx_sprite spr = { 0 };
                spr.x = px + dx_off[qi];
                spr.y = py + dy_off[qi];
                spr.tile = t & 0xFF;
                spr.flags = (PAL_ZPAC << 4) | SPRITE_FLIP_X;
                if (t >= 256) spr.flags |= 0x01;
                gfx_sprite_render(&vctx, base_spr + qi, &spr);
            }
        } else {
            /* Hide this icon off-screen */
            uint8_t qi;
            for (qi = 0; qi < 4; qi++) {
                gfx_sprite spr = { 0 };
                spr.x = 0;
                spr.y = 480;
                gfx_sprite_render(&vctx, base_spr + qi, &spr);
            }
        }
    }
}

/* Show GAME OVER text, wait ~3 seconds, then return */
static void show_game_over(void) {
    uint8_t si;
    /* Hide ZPac sprites off-screen */
    for (si = 0; si < 4; si++) {
        gfx_sprite spr = { 0 };
        spr.x = 0;
        spr.y = 480;
        gfx_sprite_render(&vctx, si, &spr);
    }
    hide_all_ghosts();

    put_text(15, 16, "GAME OVER", PAL_ZPAC);

    /* Hide life icon sprites */
    {
        uint8_t li;
        for (li = 0; li < MAX_LIFE_ICONS * 4; li++) {
            gfx_sprite spr = { 0 };
            spr.x = 0;
            spr.y = 480;
            gfx_sprite_render(&vctx, HW_SPR_LIFE_BASE + li, &spr);
        }
    }

    fruit_hud_hide();

    /* Wait ~3 seconds (180 frames), shorter in demo mode */
    {
        uint8_t wait = demo_mode ? 75 : 225;
        uint8_t ff;
        for (ff = 0; ff < wait; ff++) {
            gfx_wait_vblank(&vctx);
        }
    }
}

/* Death animation tile table: 12 frames x 4 quadrants.
 * Frame 0 = initial "opening", frame 11 = fully vanished (all blank). */
static const uint16_t death_tiles[12][4] = {
    { SPR_PAC_DEATH_0_TL,  SPR_PAC_DEATH_0_TR,  SPR_PAC_DEATH_0_BL,  SPR_PAC_DEATH_0_BR  },
    { SPR_PAC_DEATH_1_TL,  SPR_PAC_DEATH_1_TR,  SPR_PAC_DEATH_1_BL,  SPR_PAC_DEATH_1_BR  },
    { SPR_PAC_DEATH_2_TL,  SPR_PAC_DEATH_2_TR,  SPR_PAC_DEATH_2_BL,  SPR_PAC_DEATH_2_BR  },
    { SPR_PAC_DEATH_3_TL,  SPR_PAC_DEATH_3_TR,  SPR_PAC_DEATH_3_BL,  SPR_PAC_DEATH_3_BR  },
    { SPR_PAC_DEATH_4_TL,  SPR_PAC_DEATH_4_TR,  SPR_PAC_DEATH_4_BL,  SPR_PAC_DEATH_4_BR  },
    { SPR_PAC_DEATH_5_TL,  SPR_PAC_DEATH_5_TR,  SPR_PAC_DEATH_5_BL,  SPR_PAC_DEATH_5_BR  },
    { SPR_PAC_DEATH_6_TL,  SPR_PAC_DEATH_6_TR,  SPR_PAC_DEATH_6_BL,  SPR_PAC_DEATH_6_BR  },
    { SPR_PAC_DEATH_7_TL,  SPR_PAC_DEATH_7_TR,  SPR_PAC_DEATH_7_BL,  SPR_PAC_DEATH_7_BR  },
    { SPR_PAC_DEATH_8_TL,  SPR_PAC_DEATH_8_TR,  SPR_PAC_DEATH_8_BL,  SPR_PAC_DEATH_8_BR  },
    { SPR_PAC_DEATH_9_TL,  SPR_PAC_DEATH_9_TR,  SPR_PAC_DEATH_9_BL,  SPR_PAC_DEATH_9_BR  },
    { SPR_PAC_DEATH_10_TL, SPR_PAC_DEATH_10_TR, SPR_PAC_DEATH_10_BL, SPR_PAC_DEATH_10_BR },
    { SPR_PAC_DEATH_11_TL, SPR_PAC_DEATH_11_TR, SPR_PAC_DEATH_11_BL, SPR_PAC_DEATH_11_BR }
};

/* Render a single death animation frame on ZPac sprite slots 0-3.
 * Uses ZPac's current position (does not move). */
static void render_death_frame(uint8_t frame) {
    uint16_t px;
    uint16_t py;
    uint16_t sx;
    uint16_t sy;
    uint8_t i;

    /* Recompute ZPac pixel position (same formula as update_zpac_sprites) */
    px = ((uint16_t)zpac.tile_x << 3) + ((uint16_t)zpac.tile_x << 2) + zpac.sub_x;
    py = ((uint16_t)zpac.tile_y << 3) + ((uint16_t)zpac.tile_y << 2) + zpac.sub_y;
    sx = 144 + px + SPR_OFF_X;
    sy =  48 + py + 1;  /* SPR_OFF_Y = 1 */

    for (i = 0; i < 4; i++) {
        uint16_t t = death_tiles[frame][i];
        gfx_sprite spr = { 0 };
        spr.x = sx + ((i & 1) ? 16 : 0);   /* 0 or 16 for left/right */
        spr.y = sy + ((i & 2) ? 16 : 0);   /* 0 or 16 for top/bottom */
        spr.tile = t & 0xFF;
        spr.flags = (PAL_ZPAC << 4);
        if (t >= 256) spr.flags |= 0x01;
        gfx_sprite_render(&vctx, i, &spr);
    }
}

/* Award extra life at 10,000 points (once per game) */
static void check_extra_life(void) {
    if (!extra_life_given && score >= 10000UL) {
        extra_life_given = 1;
        lives++;
        lives_render();
    }
}

/* Death sequence: freeze, animate, decrement lives, respawn or GAME OVER.
 * Returns 1 if GAME OVER, 0 if game continues. */
static uint8_t death_reset(void) {
    uint8_t f;
    uint8_t frame;
    sound_stop_all();

    /* Phase 1: Freeze ~1 second — everyone visible and still */
    update_zpac_sprites();
    ghosts_render();
    for (f = 0; f < 31; f++) {
        gfx_wait_end_vblank(&vctx);
        gfx_wait_vblank(&vctx);
    }

    /* Phase 2: Hide ghosts, keep ZPac visible */
    hide_all_ghosts();
    update_zpac_sprites();

    /* Start death jingle */
    if (!demo_mode) sound_death_start();

    /* Phase 3: Death animation — 12 frames, ~10 ticks each.
     * Sound updates every vblank (120 ticks total, 1 per vblank). */
    for (frame = 0; frame < 12; frame++) {
        uint8_t tick;
        render_death_frame(frame);
        for (tick = 0; tick < 10; tick++) {
            gfx_wait_end_vblank(&vctx);
            gfx_wait_vblank(&vctx);
            if (!demo_mode) sound_death_update(0);
        }
    }

    /* Silence death jingle */
    if (!demo_mode) sound_death_stop();

    /* Phase 4: ZPac gone — hide sprites, brief pause ~0.5s */
    {
        uint8_t si;
        for (si = 0; si < 4; si++) {
            gfx_sprite spr = { 0 };
            spr.x = 0;
            spr.y = 480;
            gfx_sprite_render(&vctx, si, &spr);
        }
    }
    for (f = 0; f < 15; f++) {
        gfx_wait_end_vblank(&vctx);
        gfx_wait_vblank(&vctx);
    }

    /* Phase 5: Decrement lives and update HUD */
    if (lives > 0) {
        lives--;
    }
    lives_render();
    fruit_hud_render();

    /* Phase 6: Check GAME OVER */
    if (lives == 0) {
        show_game_over();
        return 1;  /* signal game over to caller */
    }

    /* Phase 7: Respawn */
    fruit_hide();
    ghosts_init();

    /* Arcade behavior: activate global dot counter after life lost */
    global_dot_counter_active = 1;
    global_dot_counter = 0;

    zpac.tile_x = 14;
    zpac.tile_y = 23;
    zpac.sub_x = 6;
    zpac.sub_y = 6;
    zpac.dir_current = DIR_LEFT;
    zpac.dir_desired = DIR_LEFT;
    zpac.speed_acc = 0;
    zpac.anim_tick = 0;
    zpac.eat_pause = 0;

    /* Re-apply level 256 corruption on respawn (arcade behavior:
     * fruit drawing routine re-runs, re-corrupting the right half
     * and re-placing the 8 dots) */
    if (is_level_256) {
        uint8_t r, c;
        uint8_t left_dots = 0;
        /* Count dots remaining in the left half */
        for (r = 0; r < 31; r++) {
            for (c = 0; c < 14; c++) {
                if (maze_logic[r][c] & CELL_DOT) left_dots++;
                if (maze_logic[r][c] & CELL_ENERGIZER) left_dots++;
            }
        }
        /* Re-corrupt right half (logic + tilemap) */
        level256_apply();
        /* Correct dot_count: left_dots + 8 new dots on right */
        dot_count = left_dots + 8;
    }

    update_zpac_sprites();
    ghosts_render();

    /* Show READY! for ~2 seconds */
    put_text(17, 16, "READY!", PAL_ZPAC);
    for (f = 0; f < 63; f++) {
        gfx_wait_end_vblank(&vctx);
        gfx_wait_vblank(&vctx);
    }
    restore_ready_tiles();
    if (!demo_mode) sound_start_siren();
    return 0;  /* game continues */
}

/* Hide all 64 hardware sprites off-screen */
static void clean_all_sprites(void) {
    uint8_t i;
    for (i = 0; i < 64; i++) {
        gfx_sprite spr = { 0 };
        spr.x = 0;
        spr.y = 480;
        gfx_sprite_render(&vctx, i, &spr);
    }
}

/* Render "HIGH SCORE" label */
static void render_hud_common(void) {
    put_text(15, 1, "HIGH SCORE", PAL_FONT);
}

/* Render high score value at row 2, cols 17-23 */
static void high_score_render(void) {
    uint32_t val = high_score;
    uint8_t col = 17;
    uint8_t started = 0;
    uint8_t i;
    static const uint32_t p10[7] = {
        1000000UL,100000UL,10000UL,1000UL,100UL,10UL,1UL
    };
    for (i = 0; i < 7; i++) {
        uint8_t digit = 0;
        while (val >= p10[i]) { val -= p10[i]; digit++; }
        if (digit > 0 || started || i >= 5) {
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

/* Render "CREDIT" + number at row 28 */
static void credit_render(void) {
    put_text(11, 28, "CREDIT  ", PAL_FONT);
    if (credits < 10) {
        put_char(19, 28, FONT_0 + credits, PAL_FONT);
    } else {
        put_char(18, 28, FONT_0 + (credits / 10), PAL_FONT);
        put_char(19, 28, FONT_0 + (credits % 10), PAL_FONT);
    }
}

/* --- DEMO MODE AI --- */

/* Simple AI for demo mode: prefer direction with nearest dot,
 * avoid reversing unless no other option. */
/* Energizer positions in the 28x31 logic grid */
static const uint8_t energizer_pos[4][2] = {
    { 1, 3}, {26, 3}, { 1, 23}, {26, 23}
};

static uint8_t demo_ai_read(void) {
    uint8_t d, gi;
    uint8_t best_dir;
    uint16_t best_dist;
    uint8_t reverse;
    uint8_t target_x, target_y;
    uint8_t has_target;
    uint8_t walkable_dirs;

    /* Only decide at tile center */
    if (zpac.sub_x != 6 || zpac.sub_y != 6)
        return DIR_NONE;

    reverse = DIR_OPPOSITE(zpac.dir_current);
    has_target = 0;
    target_x = zpac.tile_x;
    target_y = zpac.tile_y;

    /* === PRIORITY 1: Chase frightened ghost === */
    {
        uint16_t closest_fright_dist = 0xFFFF;
        for (gi = 0; gi < NUM_GHOSTS; gi++) {
            uint16_t dx2, dy2, d2;
            int8_t fdx, fdy;
            if (ghosts[gi].state != GHOST_STATE_FRIGHTENED) continue;
            fdx = (int8_t)ghosts[gi].tile_x - (int8_t)zpac.tile_x;
            fdy = (int8_t)ghosts[gi].tile_y - (int8_t)zpac.tile_y;
            dx2 = (uint16_t)(fdx * fdx);
            dy2 = (uint16_t)(fdy * fdy);
            d2 = dx2 + dy2;
            if (d2 < closest_fright_dist) {
                closest_fright_dist = d2;
                target_x = ghosts[gi].tile_x;
                target_y = ghosts[gi].tile_y;
                has_target = 1;
            }
        }
    }

    /* === PRIORITY 2: Navigate to nearest energizer === */
    if (!has_target) {
        uint16_t closest_enrg_dist = 0xFFFF;
        uint8_t ei;
        for (ei = 0; ei < 4; ei++) {
            uint16_t dx2, dy2, d2;
            int8_t edx, edy;
            uint8_t ex = energizer_pos[ei][0];
            uint8_t ey = energizer_pos[ei][1];
            if (!(maze_logic[ey][ex] & CELL_ENERGIZER)) continue;
            edx = (int8_t)ex - (int8_t)zpac.tile_x;
            edy = (int8_t)ey - (int8_t)zpac.tile_y;
            dx2 = (uint16_t)(edx * edx);
            dy2 = (uint16_t)(edy * edy);
            d2 = dx2 + dy2;
            if (d2 < closest_enrg_dist) {
                closest_enrg_dist = d2;
                target_x = ex;
                target_y = ey;
                has_target = 1;
            }
        }
    }

    /* === PRIORITY 3: Find nearest dot (scan 8 tiles in each dir) === */
    if (!has_target) {
        uint16_t closest_dot_dist = 0xFFFF;
        for (d = 0; d < 4; d++) {
            int8_t cx = (int8_t)zpac.tile_x;
            int8_t cy = (int8_t)zpac.tile_y;
            uint8_t step;
            for (step = 0; step < 8; step++) {
                uint8_t cell;
                int8_t ddx, ddy;
                uint16_t dx2, dy2, d2;
                cx += dir_dx[d];
                cy += dir_dy[d];
                if (cx < 0) cx = 27;
                if (cx > 27) cx = 0;
                if (cy < 0 || cy > 30) break;
                cell = maze_logic[(uint8_t)cy][(uint8_t)cx];
                if (!(cell & CELL_WALKABLE)) break;
                if (cell & CELL_DOT) {
                    ddx = cx - (int8_t)zpac.tile_x;
                    ddy = cy - (int8_t)zpac.tile_y;
                    dx2 = (uint16_t)(ddx * ddx);
                    dy2 = (uint16_t)(ddy * ddy);
                    d2 = dx2 + dy2;
                    if (d2 < closest_dot_dist) {
                        closest_dot_dist = d2;
                        target_x = (uint8_t)cx;
                        target_y = (uint8_t)cy;
                        has_target = 1;
                    }
                    break;  /* found a dot in this direction */
                }
            }
        }
    }

    /* If no target found, just keep going */
    if (!has_target) {
        if (can_walk(zpac.tile_x, zpac.tile_y, zpac.dir_current))
            return DIR_NONE;
        /* Stuck: try anything */
        for (d = 0; d < 4; d++) {
            if (can_walk(zpac.tile_x, zpac.tile_y, d)) return d;
        }
        return DIR_NONE;
    }

    /* === DIRECTION SELECTION (same algorithm as ghost AI) ===
     * For each walkable non-reverse direction, compute distance²
     * from the NEXT tile to the target. Pick the minimum. */
    best_dir = DIR_NONE;
    best_dist = 0xFFFF;
    walkable_dirs = 0;

    for (d = 0; d < 4; d++) {
        int8_t nx, ny;
        int8_t ddx, ddy;
        uint16_t dx2, dy2, d2;
        uint8_t ghost_adjacent;

        if (d == reverse) continue;
        if (!can_walk(zpac.tile_x, zpac.tile_y, d)) continue;
        walkable_dirs++;

        nx = (int8_t)zpac.tile_x + dir_dx[d];
        ny = (int8_t)zpac.tile_y + dir_dy[d];
        if (nx < 0) nx = 27;
        if (nx > 27) nx = 0;

        /* Ghost danger: only check IMMEDIATE next tile (1 tile ahead).
         * Skip this check if we're chasing frightened ghosts. */
        ghost_adjacent = 0;
        {
            uint8_t fright_exists = 0;
            for (gi = 0; gi < NUM_GHOSTS; gi++) {
                if (ghosts[gi].state == GHOST_STATE_FRIGHTENED) {
                    fright_exists = 1;
                    break;
                }
            }
            if (!fright_exists) {
                for (gi = 0; gi < NUM_GHOSTS; gi++) {
                    if (ghosts[gi].state == GHOST_STATE_HOUSE) continue;
                    if (ghosts[gi].state == GHOST_STATE_LEAVEHOUSE) continue;
                    if (ghosts[gi].state == GHOST_STATE_EYES) continue;
                    if (ghosts[gi].state == GHOST_STATE_ENTERHOUSE) continue;
                    if (ghosts[gi].state == GHOST_STATE_FRIGHTENED) continue;
                    if (ghosts[gi].tile_x == (uint8_t)nx &&
                        ghosts[gi].tile_y == (uint8_t)ny) {
                        ghost_adjacent = 1;
                        break;
                    }
                }
            }
        }

        if (ghost_adjacent) continue;  /* skip this direction entirely */

        /* Distance² from next tile to target */
        ddx = nx - (int8_t)target_x;
        ddy = ny - (int8_t)target_y;
        dx2 = (uint16_t)(ddx * ddx);
        dy2 = (uint16_t)(ddy * ddy);
        d2 = dx2 + dy2;

        if (d2 < best_dist) {
            best_dist = d2;
            best_dir = d;
        }
    }

    /* If all non-reverse directions have adjacent ghosts, reverse */
    if (best_dir == DIR_NONE) {
        if (can_walk(zpac.tile_x, zpac.tile_y, reverse))
            return reverse;
        return DIR_NONE;
    }

    /* If best direction is same as current, keep going */
    if (best_dir == zpac.dir_current) {
        return DIR_NONE;
    }

    return best_dir;
}

/* Restore maze tiles over "GAME OVER" text area (row 16, cols 15-23) */
static void restore_gameover_tiles(void) {
    uint8_t ri;
    for (ri = 0; ri < 10; ri++) {
        uint8_t scr_col = 15 + ri;
        uint8_t maze_col = scr_col - MAZE_OFFSET_X;
        uint8_t maze_row = 16 - MAZE_OFFSET_Y;
        if (maze_col < MAZE_COLS) {
            uint8_t tile = zpac_maze_map[maze_row][maze_col];
            uint8_t attr = zpac_maze_attr[maze_row][maze_col];
            gfx_tilemap_place(&vctx, tile, 0, scr_col, 16);
            gfx_tilemap_place(&vctx, attr, 1, scr_col, 16);
        } else {
            gfx_tilemap_place(&vctx, (uint8_t)(TILE_BLANK & 0xFF), 0, scr_col, 16);
            gfx_tilemap_place(&vctx, TILE_BLANK_ATTR, 1, scr_col, 16);
        }
    }
}

static void demo_playing_init(void) {
    demo_mode = 1;
    demo_frame_count = 0;
    sound_stop_all();

    game_level = 0;
    lives = 1;
    extra_life_given = 1;
    score = 0;

    init_maze_logic();
    dots_init();
    ghosts_init();
    update_level_speeds();
    fruit_init();

    clean_tilemap();
    clean_all_sprites();
    redraw_maze();

    /* Minimal HUD */
    render_hud_common();
    put_text(11, 1, "1UP", PAL_FONT);
    high_score_render();

    /* Show "GAME  OVER" — stays visible for entire demo */
    put_text(15, 16, "GAME  OVER", PAL_FONT);

    /* Wait ~2 seconds, check coin */
    {
        uint8_t f;
        for (f = 0; f < 150; f++) {
            gfx_wait_vblank(&vctx);
            {
                uint8_t key = input_read_menu();
                if (key == INPUT_COIN) {
                    if (credits < 99) credits++;
                    pending_coin_sound = 1;
                    demo_mode = 0;
                    return;
                }
                if (key == INPUT_START && credits > 0) {
                    credits--;
                    pending_start = 1;
                    demo_mode = 0;
                    return;
                }
            }
        }
    }
    if (!demo_mode) return;

    /* DO NOT clear GAME OVER text — it stays on screen */
    /* DO NOT show READY! — go straight to gameplay */

    /* Position zpac */
    zpac.tile_x = 14;
    zpac.tile_y = 23;
    zpac.sub_x = 6;
    zpac.sub_y = 6;
    zpac.speed_acc = 0;
    zpac.dir_current = DIR_LEFT;
    zpac.dir_desired = DIR_LEFT;
    zpac.anim_tick = 0;
    zpac.eat_pause = 0;

    update_zpac_sprites();
    ghosts_render();

    /* No siren — demo is silent */
}

/* --- ATTRACT MODE CHASE --- */

#define ATTRACT_STATE_IDLE     0
#define ATTRACT_STATE_SETUP    1
#define ATTRACT_STATE_CHASE    2
#define ATTRACT_STATE_REVERSE  3
#define ATTRACT_STATE_EATING   4
#define ATTRACT_STATE_EXIT     5
#define ATTRACT_STATE_COOLDOWN 6

static uint8_t  attract_state;
static uint16_t attract_timer;
static int16_t  attract_pac_x;
static int16_t  attract_ghost_x[4];
static uint8_t  attract_ghost_alive[4];
static uint8_t  attract_eating_idx;
static uint8_t  attract_eat_pause;
static uint8_t  attract_anim_tick;
static uint8_t  attract_cycle_count;
static uint8_t  attract_demo_requested;

#define ATTRACT_Y           400
#define ATTRACT_PILL_COL     11
#define ATTRACT_PILL_PX     (ATTRACT_PILL_COL * 16 + 8)

#define ATTRACT_PAC_SPEED_CHASE    2
#define ATTRACT_GHOST_SPEED_CHASE  2
#define ATTRACT_PAC_SPEED_EAT      3
#define ATTRACT_GHOST_SPEED_FRIGHT  1

#define ATTRACT_PAC_START_X        540
#define ATTRACT_GHOST_GAP           36
#define ATTRACT_GHOST_OFFSET        60

static void attract_set_sprite(uint8_t base_idx,
    uint16_t t_tl, uint16_t t_tr, uint16_t t_bl, uint16_t t_br,
    uint8_t pal, uint8_t flip_flags,
    int16_t px, uint16_t py)
{
    uint16_t tiles[4];
    int16_t dxo[4];
    uint16_t dyo[4];
    uint8_t tile_map[4];
    uint8_t i;

    tiles[0] = t_tl; tiles[1] = t_tr;
    tiles[2] = t_bl; tiles[3] = t_br;
    dxo[0] = 0; dxo[1] = 16; dxo[2] = 0; dxo[3] = 16;
    dyo[0] = 0; dyo[1] = 0;  dyo[2] = 16; dyo[3] = 16;

    if (flip_flags & SPRITE_FLIP_X) {
        tile_map[0] = 1; tile_map[1] = 0;
        tile_map[2] = 3; tile_map[3] = 2;
    } else {
        tile_map[0] = 0; tile_map[1] = 1;
        tile_map[2] = 2; tile_map[3] = 3;
    }

    for (i = 0; i < 4; i++) {
        gfx_sprite spr = { 0 };
        if (px + dxo[i] < 0 || px + dxo[i] > 800) {
            spr.x = 0; spr.y = 480;
        } else {
            spr.x = (uint16_t)(px + dxo[i]);
            spr.y = py + dyo[i];
        }
        spr.tile = tiles[tile_map[i]] & 0xFF;
        spr.flags = (pal << 4) | flip_flags;
        if (tiles[tile_map[i]] >= 256) spr.flags |= 0x01;
        gfx_sprite_render(&vctx, base_idx + i, &spr);
    }
}

static void attract_hide_sprite(uint8_t base_idx) {
    uint8_t i;
    for (i = 0; i < 4; i++) {
        gfx_sprite spr = { 0 };
        spr.x = 0; spr.y = 480;
        gfx_sprite_render(&vctx, base_idx + i, &spr);
    }
}

/* zpac animation frames for attract mode */
static const uint16_t attract_pac_frames[3][4] = {
    { SPR_PAC_RIGHT_WIDE_TL, SPR_PAC_RIGHT_WIDE_TR,
      SPR_PAC_RIGHT_WIDE_BL, SPR_PAC_RIGHT_WIDE_BR },
    { SPR_PAC_RIGHT_MED_TL, SPR_PAC_RIGHT_MED_TR,
      SPR_PAC_RIGHT_MED_BL, SPR_PAC_RIGHT_MED_BR },
    { SPR_PAC_CLOSED_TL, SPR_PAC_CLOSED_TR,
      SPR_PAC_CLOSED_BL, SPR_PAC_CLOSED_BR }
};
static const uint8_t attract_anim_map[4] = {0, 1, 2, 1};

static void attract_render_pac(uint8_t flip) {
    uint8_t phase = attract_anim_map[(attract_anim_tick >> 3) & 3];
    attract_set_sprite(0,
        attract_pac_frames[phase][0], attract_pac_frames[phase][1],
        attract_pac_frames[phase][2], attract_pac_frames[phase][3],
        PAL_ZPAC, flip ? SPRITE_FLIP_X : 0,
        attract_pac_x, ATTRACT_Y);
}

static const uint16_t attract_ghost_left[2][4] = {
    { SPR_GHOST_LEFT_F0_TL, SPR_GHOST_LEFT_F0_TR,
      SPR_GHOST_LEFT_F0_BL, SPR_GHOST_LEFT_F0_BR },
    { SPR_GHOST_LEFT_F1_TL, SPR_GHOST_LEFT_F1_TR,
      SPR_GHOST_LEFT_F1_BL, SPR_GHOST_LEFT_F1_BR }
};
static const uint16_t attract_fright[2][4] = {
    { SPR_FRIGHT_F0_TL, SPR_FRIGHT_F0_TR,
      SPR_FRIGHT_F0_BL, SPR_FRIGHT_F0_BR },
    { SPR_FRIGHT_F1_TL, SPR_FRIGHT_F1_TR,
      SPR_FRIGHT_F1_BL, SPR_FRIGHT_F1_BR }
};
static const uint8_t attract_gpals[4] = {
    PAL_BLINKY, PAL_PINKY, PAL_INKY, PAL_CLYDE
};
static const uint16_t attract_score_tiles[4][4] = {
    { SPR_SCORE_200_TL, SPR_SCORE_200_TR,
      SPR_SCORE_200_BL, SPR_SCORE_200_BR },
    { SPR_SCORE_400_TL, SPR_SCORE_400_TR,
      SPR_SCORE_400_BL, SPR_SCORE_400_BR },
    { SPR_SCORE_800_TL, SPR_SCORE_800_TR,
      SPR_SCORE_800_BL, SPR_SCORE_800_BR },
    { SPR_SCORE_1600_TL, SPR_SCORE_1600_TR,
      SPR_SCORE_1600_BL, SPR_SCORE_1600_BR }
};

static void attract_update(void) {
    uint8_t gi;
    uint8_t gframe;
    attract_anim_tick++;

    switch (attract_state) {
        case ATTRACT_STATE_IDLE:
            return;

        case ATTRACT_STATE_SETUP:
            /* Place pill on tilemap */
            put_char(ATTRACT_PILL_COL, 25, TILE_INTRO_PILL, PAL_MAZE);

            attract_pac_x = ATTRACT_PAC_START_X;
            for (gi = 0; gi < 4; gi++) {
                attract_ghost_x[gi] = ATTRACT_PAC_START_X +
                    ATTRACT_GHOST_OFFSET + (int16_t)gi * ATTRACT_GHOST_GAP;
                attract_ghost_alive[gi] = 1;
            }
            attract_eating_idx = 0;
            attract_eat_pause = 0;
            attract_anim_tick = 0;
            attract_state = ATTRACT_STATE_CHASE;
            break;

        case ATTRACT_STATE_CHASE:
            /* zpac moves left */
            attract_pac_x -= ATTRACT_PAC_SPEED_CHASE;

            /* Ghosts move left, slightly faster (extra step every 3 frames) */
            for (gi = 0; gi < 4; gi++) {
                attract_ghost_x[gi] -= ATTRACT_GHOST_SPEED_CHASE;
                if ((attract_anim_tick % 5) == 0) {
                    attract_ghost_x[gi] -= 1;
                }
            }

            /* Render zpac facing left */
            attract_render_pac(1);

            /* Render ghosts facing left with normal palettes */
            gframe = (attract_anim_tick >> 3) & 1;
            for (gi = 0; gi < 4; gi++) {
                attract_set_sprite(4 + gi * 4,
                    attract_ghost_left[gframe][0], attract_ghost_left[gframe][1],
                    attract_ghost_left[gframe][2], attract_ghost_left[gframe][3],
                    attract_gpals[gi], 0,
                    attract_ghost_x[gi], ATTRACT_Y);
            }

            /* zpac reaches the pill? */
            if (attract_pac_x <= ATTRACT_PILL_PX) {
                /* Remove pill from tilemap */
                gfx_tilemap_place(&vctx, (uint8_t)(TILE_BLANK & 0xFF),
                    0, ATTRACT_PILL_COL, 25);
                gfx_tilemap_place(&vctx, TILE_BLANK_ATTR,
                    1, ATTRACT_PILL_COL, 25);
                attract_state = ATTRACT_STATE_REVERSE;
                attract_timer = 0;
            }
            break;

        case ATTRACT_STATE_REVERSE:
            /* Brief pause (10 frames) while eating pill */
            attract_timer++;
            if (attract_timer >= 13) {
                attract_state = ATTRACT_STATE_EATING;
                attract_eating_idx = 0;
                attract_eat_pause = 0;
            }
            break;

        case ATTRACT_STATE_EATING:
            /* Score display pause */
            if (attract_eat_pause > 0) {
                attract_eat_pause--;
                if (attract_eat_pause == 0) {
                    attract_hide_sprite(4 + (attract_eating_idx - 1) * 4);
                }
                /* zpac stays still during pause */
                attract_render_pac(0);
                break;
            }

            /* zpac moves right (fast) */
            attract_pac_x += ATTRACT_PAC_SPEED_EAT;

            /* Frightened ghosts move left (slow) */
            for (gi = 0; gi < 4; gi++) {
                if (attract_ghost_alive[gi]) {
                    attract_ghost_x[gi] -= ATTRACT_GHOST_SPEED_FRIGHT;
                }
            }

            /* Render zpac facing right */
            attract_render_pac(0);

            /* Render surviving ghosts as frightened (blue) */
            gframe = (attract_anim_tick >> 3) & 1;
            for (gi = 0; gi < 4; gi++) {
                if (!attract_ghost_alive[gi]) continue;
                attract_set_sprite(4 + gi * 4,
                    attract_fright[gframe][0], attract_fright[gframe][1],
                    attract_fright[gframe][2], attract_fright[gframe][3],
                    PAL_FRIGHTENED, 0,
                    attract_ghost_x[gi], ATTRACT_Y);
            }

            /* Collision: zpac catches next ghost? */
            if (attract_eating_idx < 4) {
                gi = attract_eating_idx;
                if (attract_ghost_alive[gi] &&
                    attract_pac_x + 16 >= attract_ghost_x[gi]) {
                    attract_ghost_alive[gi] = 0;

                    /* Show score sprite in place of ghost */
                    attract_set_sprite(4 + gi * 4,
                        attract_score_tiles[gi][0], attract_score_tiles[gi][1],
                        attract_score_tiles[gi][2], attract_score_tiles[gi][3],
                        PAL_GHOST_SCORE, 0,
                        attract_ghost_x[gi], ATTRACT_Y);

                    attract_eat_pause = 75;
                    attract_eating_idx++;
                }
            }

            /* All eaten? zpac exits right */
            if (attract_eating_idx >= 4 && attract_eat_pause == 0) {
                attract_state = ATTRACT_STATE_EXIT;
                attract_timer = 0;
            }
            break;

        case ATTRACT_STATE_EXIT:
            attract_pac_x += ATTRACT_PAC_SPEED_EAT;
            attract_render_pac(0);

            if (attract_pac_x > 680) {
                attract_hide_sprite(0);
                for (gi = 0; gi < 4; gi++) {
                    attract_hide_sprite(4 + gi * 4);
                }
                attract_state = ATTRACT_STATE_COOLDOWN;
                attract_timer = 0;
            }
            break;

        case ATTRACT_STATE_COOLDOWN:
            attract_timer++;
            if (attract_timer >= 225) {
                attract_cycle_count++;
                if (attract_cycle_count >= 1) {
                    attract_demo_requested = 1;
                    attract_state = ATTRACT_STATE_IDLE;
                } else {
                    attract_state = ATTRACT_STATE_SETUP;
                }
            }
            break;
    }
}

/* --- TITLE state --- */

static uint16_t title_anim_timer;

static const uint8_t title_ghost_pals[4] = {
    PAL_BLINKY, PAL_PINKY, PAL_INKY, PAL_CLYDE
};
static const char* title_names[4] = {
    "-SHADOW", "-SPEEDY", "-BASHFUL", "-POKEY"
};
static const char* title_nicks[4] = {
    "\"BLINKY\"", "\"PINKY\"", "\"INKY\"", "\"CLYDE\""
};

static void state_title_enter(void) {
    clean_tilemap();
    clean_all_sprites();
    sound_stop_all();

    /* HUD top */
    render_hud_common();
    put_text(11, 1, "1UP", PAL_FONT);
    high_score_render();

    /* Section header */
    put_text(10, 5, "CHARACTER / NICKNAME", PAL_FONT);

    /* Credit and start message (always visible) */
    credit_render();
    if (credits > 0) {
        put_text(10, 27, "PUSH SPACE TO START", PAL_FONT);
    } else {
        put_text(14, 27, "INSERT COIN", PAL_FONT);
    }
    put_text(11, 28, "\"1\" KEY - 1 COIN", PAL_FONT);

    /* Play pending coin sound from demo mode */
    if (pending_coin_sound) {
        pending_coin_sound = 0;
        sound_coin();
    }

    title_anim_timer = 0;
    attract_state = ATTRACT_STATE_IDLE;
    attract_timer = 0;
    attract_cycle_count = 0;
    attract_demo_requested = 0;
}

static uint8_t state_title_update(void) {
    uint8_t key;

    title_anim_timer++;

    /* Sequential animation: each ghost has 3 elements (image, name, nick)
     * spaced 75 frames apart. 225 frames per ghost total.
     * Ghost 0 starts at tick 75. */
    {
        uint8_t gi;
        for (gi = 0; gi < 4; gi++) {
            uint8_t row = 7 + gi * 3;
            uint8_t pal = title_ghost_pals[gi];
            uint16_t base_tick = 75 + (uint16_t)gi * 225;

            /* Ghost image appears */
            if (title_anim_timer == base_tick) {
                uint16_t tiles[6];
                uint8_t tr, tc;
                tiles[0] = GHOST_INTRO_TL; tiles[1] = GHOST_INTRO_TR;
                tiles[2] = GHOST_INTRO_ML; tiles[3] = GHOST_INTRO_MR;
                tiles[4] = GHOST_INTRO_BL; tiles[5] = GHOST_INTRO_BR;

                for (tr = 0; tr < 3; tr++) {
                    for (tc = 0; tc < 2; tc++) {
                        uint16_t tidx = tiles[tr * 2 + tc];
                        uint8_t attr = (pal << 4);
                        if (tidx >= 256) attr |= 0x01;
                        gfx_tilemap_place(&vctx,
                            (uint8_t)(tidx & 0xFF), 0,
                            10 + tc, row + tr);
                        gfx_tilemap_place(&vctx,
                            attr, 1,
                            10 + tc, row + tr);
                    }
                }
            }

            /* Character name appears (75 frames after image) */
            if (title_anim_timer == base_tick + 75) {
                put_text(14, row + 1, title_names[gi], pal);
            }

            /* Nickname appears (150 frames after image) */
            if (title_anim_timer == base_tick + 150) {
                put_text(23, row + 1, title_nicks[gi], pal);
            }
        }
    }

    /* Dot scoring info after all ghosts.
     * Last nickname at tick 75 + 3*225 + 150 = 900.
     * Dot at 975, pill at 1050. */
    if (title_anim_timer == 975) {
        put_char(14, 20, TILE_INTRO_DOT, PAL_MAZE);
        put_text(16, 20, "10 PTS", PAL_FONT);
    }

    if (title_anim_timer == 1050) {
        put_char(14, 22, TILE_INTRO_PILL, PAL_MAZE);
        put_text(16, 22, "50 PTS", PAL_FONT);
    }

    /* Start attract chase 3 sec after last text (pill at 1050) */
    if (title_anim_timer == 1275) {
        attract_state = ATTRACT_STATE_SETUP;
    }

    /* Update attract animation every frame */
    if (attract_state != ATTRACT_STATE_IDLE) {
        attract_update();
    }

    /* Transition to demo mode after attract chase completes */
    if (attract_demo_requested) {
        attract_demo_requested = 0;
        return GAME_STATE_DEMO;
    }

    /* Input: coin and start (always active) */
    key = input_read_menu();
    if (key == INPUT_COIN) {
        if (credits < 99) credits++;
        credit_render();
        sound_coin();
        put_text(10, 27, "PUSH SPACE TO START", PAL_FONT);
    }
    if (key == INPUT_START && credits > 0) {
        credits--;
        credit_render();
        return GAME_STATE_PLAYING;
    }
    return GAME_STATE_TITLE;
}

/* --- PLAYING state --- */

static void game_playing_init(void) {
    demo_mode = 0;
    game_level = 0;
    is_level_256 = 0;
    lives = 3;
    extra_life_given = 0;
    score = 0;

    init_maze_logic();
    dots_init();
    ghosts_init();
    update_level_speeds();
    fruit_init();

    clean_tilemap();
    clean_all_sprites();
    redraw_maze();
    render_hud();
    score_render();
    high_score_render();
    lives_render();
    fruit_hud_render();
    credit_render();

    zpac.tile_x = 14;
    zpac.tile_y = 23;
    zpac.sub_x = 6;
    zpac.sub_y = 6;
    zpac.speed_acc = 0;
    zpac.dir_current = DIR_LEFT;
    zpac.dir_desired = DIR_LEFT;
    zpac.anim_tick = 0;
    zpac.eat_pause = 0;

    update_zpac_sprites();
    ghosts_render();

    put_text(17, 16, "READY!", PAL_ZPAC);
    sound_play_prelude();
    restore_ready_tiles();
    sound_start_siren();
}

static void game_playing_run(void);

/* --- GAME OVER exit --- */

static void state_gameover_exit(void) {
    if (score > high_score) {
        high_score = score;
    }
}

/* Gameplay loop — runs until GAME OVER, then returns */
static void game_playing_run(void) {
    while (1) {
        uint8_t new_dir;
        uint8_t steps;
        uint8_t s;

        gfx_wait_vblank(&vctx);

        /* Demo mode: check coin and timer */
        if (demo_mode) {
            uint8_t dkey = input_read_menu();
            if (dkey == INPUT_COIN) {
                if (credits < 99) credits++;
                pending_coin_sound = 1;
                return;
            }
            if (dkey == INPUT_START && credits > 0) {
                credits--;
                pending_start = 1;
                return;
            }
            demo_frame_count++;
            if (demo_frame_count >= DEMO_DURATION) {
                /* Force death after ~50 seconds */
                uint8_t f;
                sound_stop_all();
                update_zpac_sprites();
                ghosts_render();
                for (f = 0; f < 38; f++) {
                    gfx_wait_end_vblank(&vctx);
                    gfx_wait_vblank(&vctx);
                }
                hide_all_ghosts();
                for (f = 0; f < 8; f++) {
                    uint8_t tick;
                    render_death_frame(f);
                    for (tick = 0; tick < 10; tick++) {
                        gfx_wait_end_vblank(&vctx);
                        gfx_wait_vblank(&vctx);
                    }
                }
                {
                    uint8_t si;
                    for (si = 0; si < 4; si++) {
                        gfx_sprite spr = { 0 };
                        spr.x = 0; spr.y = 480;
                        gfx_sprite_render(&vctx, si, &spr);
                    }
                }
                for (f = 0; f < 38; f++) {
                    gfx_wait_end_vblank(&vctx);
                    gfx_wait_vblank(&vctx);
                }
                return;
            }
        }

        sound_update();
        dots_update();
        ghosts_update();
        fruit_update();

        /* In demo mode, keep GAME OVER text persistent on tilemap.
         * Re-draw every frame in case dots_update overwrites tiles. */
        if (demo_mode) {
            put_text(15, 16, "GAME  OVER", PAL_FONT);
        }

        /* Global freeze: both ZPac and ghosts stop during ghost-eat pause.
         * ghosts_update() already handles ghost freeze + timer decrement.
         * Here we freeze ZPac too: skip input, movement, everything. */
        if (ghost_eaten_freeze > 0) {
            update_zpac_sprites();
            ghosts_render();
            continue;
        }

        /* 0. Early collision check: detect ghost walking into stationary ZPac */
        {
            uint8_t hit = ghosts_check_collision(zpac.tile_x, zpac.tile_y);
            if (hit == 0xFF) {
                /* No collision */
            } else if (hit & 0x80) {
                /* Ate a frightened ghost — score already updated.
                 * Update score display and continue (freeze handled in ghosts_update). */
                score_render();
                check_extra_life();
                if (!demo_mode) sound_ghost_eaten();
                update_zpac_sprites();
                ghosts_render();
                continue;
            } else {
                if (death_reset()) break;
                continue;
            }
        }

        /* 1. Input (AI in demo mode) */
        if (demo_mode) {
            new_dir = demo_ai_read();
        } else {
            new_dir = input_read();
        }
        if (new_dir != DIR_NONE) {
            zpac.dir_desired = new_dir;
        }

        /* 2. At tile center: direction change + eating */
        if (zpac.sub_x == 6 && zpac.sub_y == 6) {
            if (can_walk(zpac.tile_x, zpac.tile_y, zpac.dir_desired)) {
                zpac.dir_current = zpac.dir_desired;
            }

            {
                uint8_t ate = dots_check_eat();
                if (ate) {
                    score_render();
                    check_extra_life();
                    /* Check fruit spawn */
                    {
                        uint8_t dots_eaten = (uint8_t)(244 - dot_count);
                        fruit_check_spawn(dots_eaten);
                    }
                    if (!demo_mode) sound_waka();
                    if (ate == 2) {
                        ghosts_enter_frightened();
                        if (!demo_mode) sound_start_fright();
                    }

                    /* Check level complete: all dots eaten */
                    if (dot_count == 0) {
                        if (demo_mode) {
                            sound_stop_all();
                            return;
                        }
                        level_complete_sequence();
                        continue;  /* restart main loop with new level */
                    }
                }
            }

            if (!can_walk(zpac.tile_x, zpac.tile_y, zpac.dir_current)) {
                zpac.anim_tick = 4;
                update_zpac_sprites();
                ghosts_render();
                continue;
            }
        }

        /* 3. Eating pause */
        if (zpac.eat_pause > 0) {
            zpac.eat_pause--;
            update_zpac_sprites();
            ghosts_render();
            continue;
        }

        /* 4. Speed accumulator fixed-point 8.8 */
        /* Select ZPac speed: frightened = faster, otherwise normal */
        {
            uint16_t pac_speed;
            if (fright_timer > 0) {
                pac_speed = cur_spd.pac_fright;
            } else {
                pac_speed = cur_spd.pac_normal;
            }
            zpac.speed_acc += pac_speed;
        }
        steps = (uint8_t)(zpac.speed_acc >> 8);
        zpac.speed_acc &= 0xFF;

        /* 5. Move 1px at a time (intercept tile center) */
        for (s = 0; s < steps; s++) {
            int8_t dx = dir_dx[zpac.dir_current];
            int8_t dy = dir_dy[zpac.dir_current];
            int8_t new_sub_x, new_sub_y;

            if (s > 0 && zpac.sub_x == 6 && zpac.sub_y == 6) {
                break;
            }

            new_sub_x = (int8_t)zpac.sub_x + dx;
            new_sub_y = (int8_t)zpac.sub_y + dy;

            if (new_sub_x >= 12) {
                new_sub_x -= 12;
                if (zpac.tile_x >= 27) zpac.tile_x = 0;
                else zpac.tile_x++;
            } else if (new_sub_x < 0) {
                new_sub_x += 12;
                if (zpac.tile_x == 0) zpac.tile_x = 27;
                else zpac.tile_x--;
            }

            if (new_sub_y >= 12) {
                new_sub_y -= 12;
                zpac.tile_y++;
            } else if (new_sub_y < 0) {
                new_sub_y += 12;
                zpac.tile_y--;
            }

            zpac.sub_x = (uint8_t)new_sub_x;
            zpac.sub_y = (uint8_t)new_sub_y;
        }

        /* 5b. Collision check after movement */
        {
            uint8_t hit = ghosts_check_collision(zpac.tile_x, zpac.tile_y);
            if (hit == 0xFF) {
                /* No collision */
            } else if (hit & 0x80) {
                /* Ate a frightened ghost */
                score_render();
                check_extra_life();
                if (!demo_mode) sound_ghost_eaten();
                update_zpac_sprites();
                ghosts_render();
                continue;
            } else {
                if (death_reset()) break;
                continue;
            }
        }

        /* 6. Animation */
        zpac.anim_tick++;

        /* 7. Sprite */
        update_zpac_sprites();
        ghosts_render();
        fruit_render();
    }
}

/* State machine: TITLE -> PLAYING -> (GAME OVER) -> TITLE */
void game_loop_run(void) {
    game_state = GAME_STATE_TITLE;
    credits = 0;
    high_score = 0;
    pending_coin_sound = 0;
    pending_start = 0;

    while (1) {
        switch (game_state) {
            case GAME_STATE_TITLE:
                state_title_enter();
                while (1) {
                    gfx_wait_vblank(&vctx);
                    sound_update();
                    {
                        uint8_t next = state_title_update();
                        if (next != GAME_STATE_TITLE) {
                            game_state = next;
                            break;
                        }
                    }
                }
                break;

            case GAME_STATE_PLAYING:
                game_playing_init();
                game_playing_run();
                state_gameover_exit();
                game_state = GAME_STATE_TITLE;
                break;

            case GAME_STATE_DEMO:
                demo_playing_init();
                if (demo_mode) {
                    game_playing_run();
                }
                demo_mode = 0;
                if (pending_start) {
                    /* START was pressed during demo: go straight to game */
                    pending_start = 0;
                    game_state = GAME_STATE_PLAYING;
                } else {
                    game_state = GAME_STATE_TITLE;
                }
                break;
        }
    }
}
