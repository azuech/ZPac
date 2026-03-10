#include <zvb_gfx.h>
#include "fruit.h"
#include "zpac_types.h"
#include "zpac_maze_data.h"
#include "dots.h"
#include "sound.h"

extern gfx_context vctx;
extern zpac_t zpac;
extern uint8_t game_level;

/* Fruit state */
static uint8_t fruit_active;     /* 1 = fruit visible on screen */
static uint16_t fruit_timer;     /* frames remaining before fruit disappears */
static uint8_t fruit_type;       /* current fruit type (0-7) */
static uint8_t fruit_spawned_70; /* already spawned at 70 dots this level */
static uint8_t fruit_spawned_170;/* already spawned at 170 dots this level */

/* Score popup state */
static uint8_t  popup_active;    /* 1 = score popup showing */
static uint16_t popup_timer;     /* frames remaining */

#define POPUP_DURATION  120  /* ~2 sec at 60fps */

/* Duration fruit stays on screen: ~10 seconds.
 * At ~60fps = 600 frames. Adjust if actual framerate differs. */
#define FRUIT_DURATION  600

/* Fruit position: tile (14, 17) in the logic grid.
 * tile_sub_to_pixel: tile*12 + sub = 14*12+6 = 174 for X, 17*12+6 = 210 for Y
 * sx = 144 + 174 + 1 = 319
 * sy = 48 + 210 + 1 = 259 */
#define FRUIT_PX  319
#define FRUIT_PY  259

/* Fruit tile position for collision check */
#define FRUIT_TILE_X  14
#define FRUIT_TILE_Y  17

/* Fruit HUD icons in bottom-right */
#define HW_SPR_FRUIT_HUD_BASE  44
#define MAX_FRUIT_HUD_ICONS    5
#define FRUIT_HUD_GAP          0   /* pixels between icons */

/* Fruit type for a given level (0-based) */
static uint8_t get_fruit_for_level(uint8_t level) {
    switch (level) {
        case 0:  return FRUIT_CHERRY;
        case 1:  return FRUIT_STRAWBERRY;
        case 2:
        case 3:  return FRUIT_PEACH;
        case 4:
        case 5:  return FRUIT_APPLE;
        case 6:
        case 7:  return FRUIT_GRAPES;
        case 8:
        case 9:  return FRUIT_GALAXIAN;
        case 10:
        case 11: return FRUIT_BELL;
        default: return FRUIT_KEY;
    }
}

static uint8_t get_fruit_type(void) {
    return get_fruit_for_level(game_level);
}

/* Score per fruit type */
static const uint16_t fruit_score[NUM_FRUIT_TYPES] = {
    100, 300, 500, 700, 1000, 2000, 3000, 5000
};

/* Score digits for each fruit type (max 4 chars, right-justified) */
static const char fruit_score_digits[NUM_FRUIT_TYPES][5] = {
    " 100",   /* Cherry */
    " 300",   /* Strawberry */
    " 500",   /* Peach */
    " 700",   /* Apple */
    "1000",   /* Grapes */
    "2000",   /* Galaxian */
    "3000",   /* Bell */
    "5000"    /* Key */
};

/* Tile indices per fruit type: TL, TR, BL, BR */
static const uint16_t fruit_tiles[NUM_FRUIT_TYPES][4] = {
    { SPR_FRUIT_CHERRY_TL,     SPR_FRUIT_CHERRY_TR,     SPR_FRUIT_CHERRY_BL,     SPR_FRUIT_CHERRY_BR },
    { SPR_FRUIT_STRAWBERRY_TL, SPR_FRUIT_STRAWBERRY_TR, SPR_FRUIT_STRAWBERRY_BL, SPR_FRUIT_STRAWBERRY_BR },
    { SPR_FRUIT_PEACH_TL,      SPR_FRUIT_PEACH_TR,      SPR_FRUIT_PEACH_BL,      SPR_FRUIT_PEACH_BR },
    { SPR_FRUIT_APPLE_TL,      SPR_FRUIT_APPLE_TR,      SPR_FRUIT_APPLE_BL,      SPR_FRUIT_APPLE_BR },
    { SPR_FRUIT_GRAPES_TL,     SPR_FRUIT_GRAPES_TR,     SPR_FRUIT_GRAPES_BL,     SPR_FRUIT_GRAPES_BR },
    { SPR_FRUIT_GALAXIAN_TL,   SPR_FRUIT_GALAXIAN_TR,   SPR_FRUIT_GALAXIAN_BL,   SPR_FRUIT_GALAXIAN_BR },
    { SPR_FRUIT_BELL_TL,       SPR_FRUIT_BELL_TR,       SPR_FRUIT_BELL_BL,       SPR_FRUIT_BELL_BR },
    { SPR_FRUIT_KEY_TL,        SPR_FRUIT_KEY_TR,        SPR_FRUIT_KEY_BL,        SPR_FRUIT_KEY_BR }
};

/* Palette group per fruit type */
static const uint8_t fruit_palette[NUM_FRUIT_TYPES] = {
    PAL_CHERRY,      /* Cherry */
    PAL_STRAWBERRY,  /* Strawberry */
    PAL_PEACH,       /* Peach */
    PAL_CHERRY,      /* Apple — same palette as Cherry (ROM 0x14) */
    PAL_GRAPES,      /* Grapes */
    PAL_ZPAC,        /* Galaxian — same palette as zpac (ROM 0x09) */
    PAL_BELL_KEY,    /* Bell */
    PAL_BELL_KEY     /* Key */
};

/* Show fruit score as sprites at fruit position.
 * Uses hardware sprites 20-23 (fruit slots, now free).
 * Each sprite is one font tile (16x16), placed side by side.
 * Position: centered on fruit location, shifted 8px left. */
static void show_fruit_popup(uint8_t ftype) {
    const char *digits = fruit_score_digits[ftype];
    /* Fruit pixel position: FRUIT_PX=319, FRUIT_PY=259.
     * Popup centered: 4 digits * 16px = 64px wide.
     * Center at FRUIT_PX + 16 (center of 32px fruit) - 32 = FRUIT_PX - 16
     * Shift left by ~8px more: FRUIT_PX - 24 */
    uint16_t start_x = FRUIT_PX - 24;
    uint16_t py = FRUIT_PY + 8;  /* vertically centered in fruit area */
    uint8_t i;

    popup_active = 1;
    popup_timer = POPUP_DURATION;

    for (i = 0; i < 4; i++) {
        gfx_sprite spr = { 0 };
        char c = digits[i];

        if (c == ' ') {
            /* Hide this sprite slot (blank digit) */
            spr.x = 0;
            spr.y = 480;
        } else {
            uint16_t tile_idx = FONT_0 + (uint16_t)(c - '0');
            spr.x = start_x + (uint16_t)(i << 4);  /* i * 16 */
            spr.y = py;
            spr.tile = tile_idx & 0xFF;
            spr.flags = (uint8_t)(PAL_GHOST_SCORE << 4);
            if (tile_idx >= 256) spr.flags |= 0x01;
        }
        gfx_sprite_render(&vctx, HW_SPR_FRUIT_BASE + i, &spr);
    }
}

static void clear_fruit_popup(void) {
    popup_active = 0;
    fruit_hide();  /* reuse fruit_hide to move sprites 20-23 off-screen */
}

void fruit_init(void) {
    fruit_active = 0;
    fruit_timer = 0;
    fruit_type = 0;
    fruit_spawned_70 = 0;
    fruit_spawned_170 = 0;
    if (popup_active) {
        clear_fruit_popup();
    }
    popup_active = 0;
    popup_timer = 0;
    fruit_hide();
}

void fruit_check_spawn(uint8_t dots_eaten) {
    if (fruit_active) return;  /* already showing */

    if (dots_eaten >= 70 && !fruit_spawned_70) {
        fruit_spawned_70 = 1;
        fruit_active = 1;
        fruit_timer = FRUIT_DURATION;
        fruit_type = get_fruit_type();
    }
    else if (dots_eaten >= 170 && !fruit_spawned_170) {
        fruit_spawned_170 = 1;
        fruit_active = 1;
        fruit_timer = FRUIT_DURATION;
        fruit_type = get_fruit_type();
    }
}

void fruit_update(void) {
    /* Update score popup timer (independent of fruit active state) */
    if (popup_active) {
        if (popup_timer > 0) {
            popup_timer--;
        }
        if (popup_timer == 0) {
            clear_fruit_popup();
        }
    }

    if (!fruit_active) return;

    /* Check timeout */
    if (fruit_timer > 0) {
        fruit_timer--;
    }
    if (fruit_timer == 0) {
        fruit_active = 0;
        fruit_hide();
        return;
    }

    /* Check collision with ZPac (same tile = eaten) */
    if (zpac.tile_x == FRUIT_TILE_X && zpac.tile_y == FRUIT_TILE_Y) {
        score += (uint32_t)fruit_score[fruit_type];
        fruit_active = 0;
        /* Don't call fruit_hide() — show_fruit_popup reuses the same sprites */
        show_fruit_popup(fruit_type);
        sound_fruit_eaten();
    }
}

void fruit_render(void) {
    uint8_t i;
    uint8_t pal;

    if (!fruit_active) return;
    if (popup_active) return;  /* popup uses same sprite slots */

    pal = fruit_palette[fruit_type];

    for (i = 0; i < 4; i++) {
        uint16_t t = fruit_tiles[fruit_type][i];
        gfx_sprite spr = { 0 };
        spr.x = FRUIT_PX + ((i & 1) ? 16 : 0);
        spr.y = FRUIT_PY + ((i & 2) ? 16 : 0);
        spr.tile = t & 0xFF;
        spr.flags = (pal << 4);
        if (t >= 256) spr.flags |= 0x01;
        gfx_sprite_render(&vctx, HW_SPR_FRUIT_BASE + i, &spr);
    }
}

void fruit_hide(void) {
    uint8_t i;
    for (i = 0; i < 4; i++) {
        gfx_sprite spr = { 0 };
        spr.x = 0;
        spr.y = 480;
        gfx_sprite_render(&vctx, HW_SPR_FRUIT_BASE + i, &spr);
    }
}

/* Render fruit level icons in HUD bottom-right.
 * Shows icons for the last MAX_FRUIT_HUD_ICONS levels
 * (including current level), rightmost = most recent.
 * Aligned to right edge of maze. */
void fruit_hud_render(void) {
    uint8_t num_icons;
    uint8_t first_level;
    uint8_t icon;
    /* Y position: same as life icons (row 27 area) */
    uint16_t py = 432;
    /* Right edge of maze: col 30 * 16 = 480px. Last icon right edge there. */
    uint16_t right_x = 448;

    /* How many levels to show: current level + 1, capped at MAX */
    num_icons = game_level + 1;
    if (num_icons > MAX_FRUIT_HUD_ICONS) num_icons = MAX_FRUIT_HUD_ICONS;

    /* First level to show (oldest) */
    if (game_level + 1 > MAX_FRUIT_HUD_ICONS) {
        first_level = game_level + 1 - MAX_FRUIT_HUD_ICONS;
    } else {
        first_level = 0;
    }

    for (icon = 0; icon < MAX_FRUIT_HUD_ICONS; icon++) {
        uint8_t base_spr = HW_SPR_FRUIT_HUD_BASE + (icon << 2);

        if (icon < num_icons) {
            uint8_t lvl = first_level + icon;
            uint8_t ftype = get_fruit_for_level(lvl);
            uint8_t pal = fruit_palette[ftype];
            /* Position: rightmost icon at right_x, others shift left.
             * icon 0 = leftmost (oldest), icon num_icons-1 = rightmost (newest). */
            uint16_t px = right_x - (uint16_t)(num_icons - 1 - icon) * (32 + FRUIT_HUD_GAP);
            uint8_t qi;

            for (qi = 0; qi < 4; qi++) {
                uint16_t t = fruit_tiles[ftype][qi];
                gfx_sprite spr = { 0 };
                spr.x = px + ((qi & 1) ? 16 : 0);
                spr.y = py + ((qi & 2) ? 16 : 0);
                spr.tile = t & 0xFF;
                spr.flags = (pal << 4);
                if (t >= 256) spr.flags |= 0x01;
                gfx_sprite_render(&vctx, base_spr + qi, &spr);
            }
        } else {
            /* Hide unused icon slots */
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

void fruit_hud_hide(void) {
    uint8_t i;
    for (i = 0; i < MAX_FRUIT_HUD_ICONS * 4; i++) {
        gfx_sprite spr = { 0 };
        spr.x = 0;
        spr.y = 480;
        gfx_sprite_render(&vctx, HW_SPR_FRUIT_HUD_BASE + i, &spr);
    }
}
