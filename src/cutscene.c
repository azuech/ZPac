#include <zvb_gfx.h>
#include <string.h>
#include "cutscene.h"
#include "zpac_types.h"
#include "zpac_maze_data.h"
#include "sound.h"

extern gfx_context vctx;
extern void clean_tilemap(void);
extern void put_text(uint8_t col, uint8_t row, const char* text, uint8_t pal);

/* Hardware sprite indices used during cutscenes.
 * We reuse the same slots as gameplay (no gameplay active). */
#define CS_SPR_PAC_BASE     0   /* sprites 0-3: ZPac 2x2 in Phase 1 */
#define CS_SPR_GHOST_P1     4   /* sprites 4-7: Blinky 2x2 in Phase 1 */
#define CS_SPR_BIGPAC_BASE  0   /* sprites 0-8: Big ZPac 3x3 in Phase 2 */
#define CS_SPR_GHOST_P2     9   /* sprites 9-12: Ghost 2x2 in Phase 2 */

/* Screen dimensions */
#define SCREEN_W  640
#define SCREEN_H  480

/* Y position for cutscene actors (center of screen) */
#define CS_Y  232

/* Animation frame timing (frames between animation changes) */
#define CS_ANIM_SPEED  4

/* Hide all cutscene sprites off-screen (15 covers all acts) */
static void cs_hide_sprites(void) {
    uint8_t i;
    for (i = 0; i < 15; i++) {
        gfx_sprite spr = { 0 };
        spr.x = 0;
        spr.y = 480;
        gfx_sprite_render(&vctx, i, &spr);
    }
}

/* Render a 2x2 composite sprite at (px, py) with given tiles and palette.
 * tiles[0]=TL, tiles[1]=TR, tiles[2]=BL, tiles[3]=BR.
 * flip_x: if 1, swap columns and set SPRITE_FLIP_X flag. */
static void cs_render_2x2(uint8_t base_spr, int16_t px, int16_t py,
                           const uint16_t tiles[4], uint8_t pal,
                           uint8_t flip_x)
{
    uint8_t i;
    /* Quadrant offsets: TL(0,0) TR(16,0) BL(0,16) BR(16,16) */
    static const int8_t dx[4] = { 0, 16, 0, 16 };
    static const int8_t dy[4] = { 0, 0, 16, 16 };

    /* Tile mapping for flip_x: swap columns */
    uint8_t tile_map[4];
    if (flip_x) {
        tile_map[0] = 1; tile_map[1] = 0;
        tile_map[2] = 3; tile_map[3] = 2;
    } else {
        tile_map[0] = 0; tile_map[1] = 1;
        tile_map[2] = 2; tile_map[3] = 3;
    }

    for (i = 0; i < 4; i++) {
        gfx_sprite spr = { 0 };
        int16_t sx = px + dx[i];
        int16_t sy = py + dy[i];

        /* Off-screen check */
        if (sx < -16 || sx > 640 || sy < -16 || sy > 480) {
            spr.x = 0;
            spr.y = 480;
        } else {
            uint16_t t = tiles[tile_map[i]];
            spr.x = (uint16_t)sx;
            spr.y = (uint16_t)sy;
            spr.tile = t & 0xFF;
            spr.flags = (pal << 4);
            if (flip_x) spr.flags |= 0x08;  /* SPRITE_FLIP_X */
            if (t >= 256) spr.flags |= 0x01; /* tile bit 9 */
        }
        gfx_sprite_render(&vctx, base_spr + i, &spr);
    }
}

/* Render 3x3 composite sprite (48x48 pixels).
 * tiles[0..8] = TL,TC,TR, ML,MC,MR, BL,BC,BR. */
static void cs_render_3x3(uint8_t base_spr, int16_t px, int16_t py,
                           const uint16_t tiles[9], uint8_t pal)
{
    uint8_t i;
    for (i = 0; i < 9; i++) {
        gfx_sprite spr = { 0 };
        uint8_t grid_col = i % 3;
        uint8_t grid_row = i / 3;
        int16_t sx = px + (int16_t)(grid_col * 16);
        int16_t sy = py + (int16_t)(grid_row * 16);

        if (sx < -16 || sx > 640 || sy < -16 || sy > 480) {
            spr.x = 0;
            spr.y = 480;
        } else {
            uint16_t t = tiles[i];
            spr.x = (uint16_t)sx;
            spr.y = (uint16_t)sy;
            spr.tile = t & 0xFF;
            spr.flags = (pal << 4);
            if (t >= 256) spr.flags |= 0x01;
        }
        gfx_sprite_render(&vctx, base_spr + i, &spr);
    }
}

/* ZPac animation tiles: wide, medium, closed */
static const uint16_t pac_wide[4] = {
    SPR_PAC_RIGHT_WIDE_TL, SPR_PAC_RIGHT_WIDE_TR,
    SPR_PAC_RIGHT_WIDE_BL, SPR_PAC_RIGHT_WIDE_BR
};
static const uint16_t pac_med[4] = {
    SPR_PAC_RIGHT_MED_TL, SPR_PAC_RIGHT_MED_TR,
    SPR_PAC_RIGHT_MED_BL, SPR_PAC_RIGHT_MED_BR
};
static const uint16_t pac_closed[4] = {
    SPR_PAC_CLOSED_TL, SPR_PAC_CLOSED_TR,
    SPR_PAC_CLOSED_BL, SPR_PAC_CLOSED_BR
};

/* Phase map: 0=wide, 1=med, 2=closed, 1=med */
static const uint16_t * const pac_anim[4] = {
    pac_wide, pac_med, pac_closed, pac_med
};

/* Big ZPac animation tiles (3x3 = 9 tiles per frame) */
static const uint16_t bigpac_wide[9] = {
    SPR_BIGPAC_WIDE_TL, SPR_BIGPAC_WIDE_TC, SPR_BIGPAC_WIDE_TR,
    SPR_BIGPAC_WIDE_ML, SPR_BIGPAC_WIDE_MC, SPR_BIGPAC_WIDE_MR,
    SPR_BIGPAC_WIDE_BL, SPR_BIGPAC_WIDE_BC, SPR_BIGPAC_WIDE_BR
};
static const uint16_t bigpac_med[9] = {
    SPR_BIGPAC_MED_TL, SPR_BIGPAC_MED_TC, SPR_BIGPAC_MED_TR,
    SPR_BIGPAC_MED_ML, SPR_BIGPAC_MED_MC, SPR_BIGPAC_MED_MR,
    SPR_BIGPAC_MED_BL, SPR_BIGPAC_MED_BC, SPR_BIGPAC_MED_BR
};
static const uint16_t bigpac_closed[9] = {
    SPR_BIGPAC_CLOSED_TL, SPR_BIGPAC_CLOSED_TC, SPR_BIGPAC_CLOSED_TR,
    SPR_BIGPAC_CLOSED_ML, SPR_BIGPAC_CLOSED_MC, SPR_BIGPAC_CLOSED_MR,
    SPR_BIGPAC_CLOSED_BL, SPR_BIGPAC_CLOSED_BC, SPR_BIGPAC_CLOSED_BR
};

static const uint16_t * const bigpac_anim[4] = {
    bigpac_wide, bigpac_med, bigpac_closed, bigpac_med
};

/* Ghost left-facing animation tiles: frame 0 and frame 1 */
static const uint16_t ghost_left_f0[4] = {
    SPR_GHOST_LEFT_F0_TL, SPR_GHOST_LEFT_F0_TR,
    SPR_GHOST_LEFT_F0_BL, SPR_GHOST_LEFT_F0_BR
};
static const uint16_t ghost_left_f1[4] = {
    SPR_GHOST_LEFT_F1_TL, SPR_GHOST_LEFT_F1_TR,
    SPR_GHOST_LEFT_F1_BL, SPR_GHOST_LEFT_F1_BR
};

/* Ghost right-facing animation tiles */
static const uint16_t ghost_right_f0[4] = {
    SPR_GHOST_RIGHT_F0_TL, SPR_GHOST_RIGHT_F0_TR,
    SPR_GHOST_RIGHT_F0_BL, SPR_GHOST_RIGHT_F0_BR
};
static const uint16_t ghost_right_f1[4] = {
    SPR_GHOST_RIGHT_F1_TL, SPR_GHOST_RIGHT_F1_TR,
    SPR_GHOST_RIGHT_F1_BL, SPR_GHOST_RIGHT_F1_BR
};

/* Ghost up-facing tiles (for "looking up in shock" in Act 2) */
static const uint16_t ghost_up_f0[4] = {
    SPR_GHOST_UP_F0_TL, SPR_GHOST_UP_F0_TR,
    SPR_GHOST_UP_F0_BL, SPR_GHOST_UP_F0_BR
};

/* Ghost down-facing tiles */
static const uint16_t ghost_down_f0[4] = {
    SPR_GHOST_DOWN_F0_TL, SPR_GHOST_DOWN_F0_TR,
    SPR_GHOST_DOWN_F0_BL, SPR_GHOST_DOWN_F0_BR
};

/* Frightened ghost tiles: frame 0 and frame 1 */
static const uint16_t fright_f0[4] = {
    SPR_FRIGHT_F0_TL, SPR_FRIGHT_F0_TR,
    SPR_FRIGHT_F0_BL, SPR_FRIGHT_F0_BR
};
static const uint16_t fright_f1[4] = {
    SPR_FRIGHT_F1_TL, SPR_FRIGHT_F1_TR,
    SPR_FRIGHT_F1_BL, SPR_FRIGHT_F1_BR
};

/* Naked ghost tiles */
static const uint16_t naked_ghost_f0[4] = {
    SPR_NAKED_GHOST_F0_TL, SPR_NAKED_GHOST_F0_TR,
    SPR_NAKED_GHOST_F0_BL, SPR_NAKED_GHOST_F0_BR
};
static const uint16_t naked_ghost_f1[4] = {
    SPR_NAKED_GHOST_F1_TL, SPR_NAKED_GHOST_F1_TR,
    SPR_NAKED_GHOST_F1_BL, SPR_NAKED_GHOST_F1_BR
};

/* Custom palette for naked ghost (runtime slot 10) */
static const uint8_t naked_ghost_palette[32] = {
    0x00, 0x00, 0xFF, 0xFF, 0x1B, 0x21, 0xB5, 0xFD,
    0x00, 0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define PAL_NAKED_GHOST  10

/* Render 2x2 ghost but hide one quadrant (for torn effect).
 * skip_idx: 0=TL, 1=TR, 2=BL, 3=BR to hide. */
static void cs_render_2x2_torn(uint8_t base_spr, int16_t px, int16_t py,
                                const uint16_t tiles[4], uint8_t pal,
                                uint8_t skip_idx)
{
    uint8_t i;
    static const int8_t dx[4] = { 0, 16, 0, 16 };
    static const int8_t dy[4] = { 0, 0, 16, 16 };

    for (i = 0; i < 4; i++) {
        gfx_sprite spr = { 0 };
        if (i == skip_idx) {
            spr.x = 0;
            spr.y = 480;
        } else {
            int16_t sx = px + dx[i];
            int16_t sy = py + dy[i];
            uint16_t t = tiles[i];
            spr.x = (uint16_t)sx;
            spr.y = (uint16_t)sy;
            spr.tile = t & 0xFF;
            spr.flags = (pal << 4);
            if (t >= 256) spr.flags |= 0x01;
        }
        gfx_sprite_render(&vctx, base_spr + i, &spr);
    }
}

/* Wait N frames (also updates intermission music) */
static void cs_wait(uint16_t frames) {
    uint16_t i;
    for (i = 0; i < frames; i++) {
        gfx_wait_vblank(&vctx);
        gfx_wait_end_vblank(&vctx);
        sound_intermission_update();
    }
}

/* ================================================================
 * ACT 1: ZPac chased by Blinky, then roles reversed
 * ================================================================ */
static void cutscene_act1(void) {
    int16_t pac_x, ghost_x;
    uint16_t tick;
    const uint16_t *pac_tiles;
    const uint16_t *ghost_tiles;

    /* Start intermission music */
    sound_intermission_start();

    /* Phase 1: ZPac runs left, Blinky chases from behind.
     * ZPac starts at x=660, moves left at 2 px/frame.
     * Blinky starts at x=720, moves left at 2.5 px/frame (gains on Pac). */
    pac_x = 660;
    ghost_x = 760;
    tick = 0;

    while (pac_x > -40) {
        gfx_wait_vblank(&vctx);
        gfx_wait_end_vblank(&vctx);
        sound_intermission_update();

        /* Animate ZPac (mouth cycle) */
        pac_tiles = pac_anim[(tick / CS_ANIM_SPEED) & 3];
        cs_render_2x2(CS_SPR_PAC_BASE, pac_x, CS_Y,
                       pac_tiles, PAL_ZPAC, 1);  /* flip_x=1 for LEFT */

        /* Animate Blinky (leg cycle, left-facing) */
        ghost_tiles = ((tick / CS_ANIM_SPEED) & 1) ?
                      ghost_left_f1 : ghost_left_f0;
        cs_render_2x2(CS_SPR_GHOST_P1, ghost_x, CS_Y,
                       ghost_tiles, PAL_BLINKY, 0);

        /* Move: both go left, Blinky slightly faster (gains but never passes) */
        pac_x -= 3;
        ghost_x -= 3;
        if ((tick & 3) == 0) ghost_x--;  /* extra 1px every 4 frames */

        tick++;
    }

    /* Hide sprites, pause */
    cs_hide_sprites();
    cs_wait(60);  /* ~1 second pause */

    /* Phase 2: Blinky frightened enters from left, runs right (slow).
     * After a delay, ZPac (big placeholder) enters from left (fast).
     * ZPac gains but never catches Blinky. Both exit right. */
    ghost_x = -32;
    pac_x = -32;  /* Pac starts off-screen, enters later */
    tick = 0;

    {
        uint8_t pac_visible = 0;  /* ZPac not yet on screen */
        uint16_t pac_delay = 90;  /* frames before ZPac enters (~1.5 sec) */

        while (ghost_x < 680) {
            gfx_wait_vblank(&vctx);
            gfx_wait_end_vblank(&vctx);
            sound_intermission_update();

            /* Blinky: frightened, moving right (slow) */
            ghost_tiles = ((tick / CS_ANIM_SPEED) & 1) ?
                          fright_f1 : fright_f0;
            cs_render_2x2(CS_SPR_GHOST_P2, ghost_x, CS_Y,
                           ghost_tiles, PAL_FRIGHTENED, 0);

            /* ZPac: enters after delay */
            if (tick >= pac_delay) {
                if (!pac_visible) {
                    pac_visible = 1;
                    pac_x = -32;
                }
                {
                    const uint16_t *bp_tiles;
                    bp_tiles = bigpac_anim[(tick / CS_ANIM_SPEED) & 3];
                    cs_render_3x3(CS_SPR_BIGPAC_BASE, pac_x, CS_Y - 8,
                                   bp_tiles, PAL_ZPAC);
                }

                /* ZPac moves a bit faster than ghost */
                pac_x += 2;
                if (tick & 1) pac_x++;
            }

            /* Ghost moves slow */
            ghost_x += 2;

            tick++;
        }
    }

    /* Hide sprites, final pause */
    cs_hide_sprites();
    cs_wait(60);

    /* Silence any remaining music */
    sound_stop_all();
}

/* ================================================================
 * ACT 2: Blinky gets snagged on a nail
 * ================================================================ */
static void cutscene_act2(void) {
    int16_t pac_x, ghost_x;
    uint16_t tick;
    const uint16_t *pac_tiles;
    const uint16_t *ghost_tiles;
    uint8_t f;

    /* Nail position: center of screen */
    int16_t nail_x = 316;
    int16_t nail_y = CS_Y + 18;

    /* Draw the nail (sprite 10 — outside the 0-9 range used by actors) */
    {
        gfx_sprite spr = { 0 };
        spr.x = (uint16_t)nail_x;
        spr.y = (uint16_t)nail_y;
        spr.tile = FONT_I & 0xFF;
        spr.flags = (uint8_t)(PAL_FONT << 4) | 0x01;  /* tile bit 9: FONT_I >= 256 */
        gfx_sprite_render(&vctx, 10, &spr);
    }

    /* Start intermission music */
    sound_intermission_start();

    /* ---- Phase 1: ZPac and Blinky run left ---- */
    pac_x = 660;
    ghost_x = 760;
    tick = 0;

    while (pac_x > -40) {
        gfx_wait_vblank(&vctx);
        gfx_wait_end_vblank(&vctx);
        sound_intermission_update();

        /* Animate ZPac */
        pac_tiles = pac_anim[(tick / CS_ANIM_SPEED) & 3];
        cs_render_2x2(CS_SPR_PAC_BASE, pac_x, CS_Y,
                       pac_tiles, PAL_ZPAC, 1);

        /* Animate Blinky — normal running until near nail,
         * then start slowing down and stuttering */
        ghost_tiles = ((tick / CS_ANIM_SPEED) & 1) ?
                      ghost_left_f1 : ghost_left_f0;

        {
            int16_t dist_past_nail = nail_x - ghost_x;

            if (dist_past_nail < -40) {
                /* Far before nail: full speed */
                ghost_x -= 3;
                if ((tick & 3) == 0) ghost_x--;
            } else if (dist_past_nail < -10) {
                /* Approaching nail: start easing off slightly */
                ghost_x -= 3;
            } else if (dist_past_nail < 5) {
                /* Near/just past nail: slowing */
                ghost_x -= 2;
            } else if (dist_past_nail < 12) {
                /* Past nail: stuttering */
                if (tick & 1) ghost_x -= 1;
            } else if (dist_past_nail < 21) {
                /* Heavy stutter */
                if ((tick % 4) == 0) ghost_x -= 1;
            }
            /* >= 21: stuck */
        }

        cs_render_2x2(CS_SPR_GHOST_P1, ghost_x, CS_Y,
                       ghost_tiles, PAL_BLINKY, 0);

        pac_x -= 3;
        tick++;
    }

    /* Hide ZPac sprites */
    for (f = 0; f < 4; f++) {
        gfx_sprite spr = { 0 };
        spr.x = 0;
        spr.y = 480;
        gfx_sprite_render(&vctx, CS_SPR_PAC_BASE + f, &spr);
    }

    /* ---- Phase 2: Blinky stuck, struggling ---- */
    /* Short struggle animation: Blinky tries to move left, snaps back */
    for (f = 0; f < 20; f++) {
        gfx_wait_vblank(&vctx);
        gfx_wait_end_vblank(&vctx);
        sound_intermission_update();

        ghost_tiles = ((f / CS_ANIM_SPEED) & 1) ?
                      ghost_left_f1 : ghost_left_f0;

        /* Jitter left and back */
        {
            int16_t jitter = 0;
            if ((f & 3) == 0) jitter = -2;
            if ((f & 3) == 1) jitter = 1;
            cs_render_2x2(CS_SPR_GHOST_P1, ghost_x + jitter, CS_Y,
                           ghost_tiles, PAL_BLINKY, 0);
        }
    }

    /* ---- Phase 3: THE TEAR ---- */
    /* Blinky shifts slightly left, BR quadrant stays at nail as torn piece.
     * Gap visible between Blinky and the piece. */
    {
        int16_t torn_blinky_x = ghost_x;

        /* Place torn piece at nail (sprite 11): the BR quadrant */
        {
            gfx_sprite spr = { 0 };
            spr.x = (uint16_t)(nail_x);
            spr.y = (uint16_t)(CS_Y + 16);  /* bottom-right position */
            spr.tile = SPR_GHOST_LEFT_F0_BR;
            spr.flags = (PAL_BLINKY << 4);
            gfx_sprite_render(&vctx, 11, &spr);
        }

        /* Render Blinky looking UP (shock!) with BR quadrant hidden */
        cs_render_2x2_torn(CS_SPR_GHOST_P1, torn_blinky_x, CS_Y,
                            ghost_up_f0, PAL_BLINKY, 3);

        /* Hold "looking up in shock" for ~1 second */
        for (f = 0; f < 60; f++) {
            gfx_wait_vblank(&vctx);
            gfx_wait_end_vblank(&vctx);
            sound_intermission_update();
        }

        /* ---- Phase 4: Blinky looks right (down toward the torn piece) ---- */
        /* Swap to right-facing tiles (eyes look toward the piece on the right) */
        cs_render_2x2_torn(CS_SPR_GHOST_P1, torn_blinky_x, CS_Y,
                            ghost_right_f0, PAL_BLINKY, 3);

        /* Hold this pose for ~2 seconds */
        for (f = 0; f < 120; f++) {
            gfx_wait_vblank(&vctx);
            gfx_wait_end_vblank(&vctx);
            sound_intermission_update();
        }
    }

    /* Cleanup */
    cs_hide_sprites();
    /* Also hide sprites 10 and 11 (nail and torn piece) */
    {
        gfx_sprite spr = { 0 };
        spr.x = 0;
        spr.y = 480;
        gfx_sprite_render(&vctx, 10, &spr);
        gfx_sprite_render(&vctx, 11, &spr);
    }
    sound_stop_all();
    cs_wait(30);
}

/* ================================================================
 * ACT 3: Naked ghost runs across
 * ================================================================ */
static void cutscene_act3(void) {
    int16_t pac_x, ghost_x;
    uint16_t tick;
    const uint16_t *pac_tiles;
    const uint16_t *ghost_tiles;

    sound_intermission_start();

    /* Phase 1: ZPac + patched Blinky run left */
    pac_x = 660;
    ghost_x = 760;
    tick = 0;

    while (pac_x > -40) {
        gfx_wait_vblank(&vctx);
        gfx_wait_end_vblank(&vctx);
        sound_intermission_update();

        pac_tiles = pac_anim[(tick / CS_ANIM_SPEED) & 3];
        cs_render_2x2(CS_SPR_PAC_BASE, pac_x, CS_Y,
                       pac_tiles, PAL_ZPAC, 1);

        ghost_tiles = ((tick / CS_ANIM_SPEED) & 1) ?
                      ghost_left_f1 : ghost_left_f0;
        {
            uint8_t qi;
            static const int8_t qdx[4] = { 0, 16, 0, 16 };
            static const int8_t qdy[4] = { 0, 0, 16, 16 };
            for (qi = 0; qi < 4; qi++) {
                gfx_sprite spr = { 0 };
                int16_t sx = ghost_x + qdx[qi];
                int16_t sy = CS_Y + qdy[qi];
                uint16_t t = ghost_tiles[qi];
                uint8_t pal = PAL_BLINKY;
                if (qi == 3) pal = PAL_PEACH;
                if (sx < -16 || sx > 640) {
                    spr.x = 0; spr.y = 480;
                } else {
                    spr.x = (uint16_t)sx;
                    spr.y = (uint16_t)sy;
                    spr.tile = t & 0xFF;
                    spr.flags = (pal << 4);
                    if (t >= 256) spr.flags |= 0x01;
                }
                gfx_sprite_render(&vctx, CS_SPR_GHOST_P1 + qi, &spr);
            }
        }

        pac_x -= 3;
        ghost_x -= 3;
        if ((tick & 3) == 0) ghost_x--;
        tick++;
    }

    cs_hide_sprites();
    cs_wait(60);

    /* Phase 2: Naked ghost crosses left to right */
    gfx_palette_load(&vctx, (void*)naked_ghost_palette, 32, 160);

    ghost_x = -32;
    tick = 0;

    while (ghost_x < 680) {
        gfx_wait_vblank(&vctx);
        gfx_wait_end_vblank(&vctx);
        sound_intermission_update();

        ghost_tiles = ((tick / 8) & 1) ?
                      naked_ghost_f1 : naked_ghost_f0;
        cs_render_2x2(CS_SPR_GHOST_P1, ghost_x, CS_Y,
                       ghost_tiles, PAL_NAKED_GHOST, 0);

        ghost_x += 1;
        if (tick & 1) ghost_x++;
        tick++;
    }

    cs_hide_sprites();
    sound_stop_all();
    cs_wait(30);
}

/* ================================================================
 * Main cutscene dispatcher
 * ================================================================ */
uint8_t cutscene_check_and_play(uint8_t new_level) {
    uint8_t play_act = 0;

    /* Determine which act to play based on new level */
    if (new_level == 2)  play_act = 1;
    if (new_level == 5)  play_act = 2;
    if (new_level == 9 || new_level == 13 || new_level == 17) play_act = 3;

    if (play_act == 0) return 0;

    /* Prepare screen: black background, hide all sprites */
    sound_stop_all();
    clean_tilemap();
    cs_hide_sprites();

    /* Also hide fruit HUD sprites, life sprites, etc. */
    {
        uint8_t i;
        for (i = 15; i < 64; i++) {
            gfx_sprite spr = { 0 };
            spr.x = 0;
            spr.y = 480;
            gfx_sprite_render(&vctx, i, &spr);
        }
    }

    /* Play the cutscene */
    switch (play_act) {
        case 1: cutscene_act1(); break;
        case 2: cutscene_act2(); break;
        case 3: cutscene_act3(); break;
    }

    return 1;
}
