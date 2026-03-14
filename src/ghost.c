#include <zvb_gfx.h>
#include "ghost.h"
#include "zpac_types.h"
#include "maze_logic.h"
#include "zpac_maze_data.h"

extern gfx_context vctx;
extern zpac_t zpac;
extern uint32_t score;
extern speed_set_t cur_spd;

/* The 4 ghost structs — global, accessed from game_loop.c */
ghost_t ghosts[NUM_GHOSTS];

/* Scatter target tiles (fixed corners each ghost patrols)
 * Clamped to our 28x31 grid (max row = 30):
 * Blinky → top-right (25, 0)
 * Pinky  → top-left  (2, 0)
 * Inky   → bottom-right (27, 30)
 * Clyde  → bottom-left (0, 30) */
const uint8_t scatter_target_x[NUM_GHOSTS] = { 25,  2, 27,  0 };
const uint8_t scatter_target_y[NUM_GHOSTS] = {  0,  0, 30, 30 };

/* Arcade-accurate starting positions */
static const uint8_t start_tile_x[NUM_GHOSTS] = { 14, 14, 12, 16 };
static const uint8_t start_tile_y[NUM_GHOSTS] = { 11, 14, 14, 14 };

/* Starting directions */
static const uint8_t start_dir[NUM_GHOSTS] = {
    DIR_LEFT,   /* Blinky: above door, heading left */
    DIR_DOWN,   /* Pinky:  center house, heading down */
    DIR_UP,     /* Inky:   left slot, heading up */
    DIR_UP      /* Clyde:  right slot, heading up */
};

/* Starting states */
static const uint8_t start_state[NUM_GHOSTS] = {
    GHOST_STATE_SCATTER,  /* Blinky starts outside */
    GHOST_STATE_HOUSE,    /* Pinky starts in house */
    GHOST_STATE_HOUSE,    /* Inky starts in house */
    GHOST_STATE_HOUSE     /* Clyde starts in house */
};

/* Dot limits per ghost, per level group (Pac-Man Dossier).
 * [level_group][ghost_index]: Blinky always 0 (never in house).
 * L1: Pinky=0, Inky=30, Clyde=60
 * L2: Pinky=0, Inky=0,  Clyde=50
 * L3+: Pinky=0, Inky=0, Clyde=0 (all leave immediately) */
static const uint8_t dot_limit_table[3][NUM_GHOSTS] = {
    { 0, 0, 30, 60 },   /* [0] L1 */
    { 0, 0,  0, 50 },   /* [1] L2 */
    { 0, 0,  0,  0 },   /* [2] L3+ */
};

/* Target positions inside ghost house for ENTERHOUSE state.
 * Each ghost returns to its starting slot.
 * (Blinky and Pinky share center, Inky left, Clyde right) */
static const uint8_t house_target_x[NUM_GHOSTS] = { 14, 14, 12, 16 };
static const uint8_t house_target_y[NUM_GHOSTS] = { 14, 14, 14, 14 };

/* Scatter/Chase phase durations in frames (75fps).
 * 3 tiers: L1, L2-4, L5+.
 * Phase 7 duration 0 = permanent chase (no more transitions). */
static const uint16_t sc_durations_L1[SC_NUM_PHASES] = {
    525,   /* Scatter 7s at 75fps */
   1500,   /* Chase 20s */
    525,   /* Scatter 7s */
   1500,   /* Chase 20s */
    375,   /* Scatter 5s */
   1500,   /* Chase 20s */
    375,   /* Scatter 5s */
      0    /* Chase permanent */
};

static const uint16_t sc_durations_L2[SC_NUM_PHASES] = {
    525,   /* Scatter 7s at 75fps */
   1500,   /* Chase 20s */
    525,   /* Scatter 7s */
   1500,   /* Chase 20s */
    375,   /* Scatter 5s */
  65535,   /* Chase ~874s (uint16_t max, arcade ~1033s) */
      1,   /* Scatter 1/75s (reversal only) */
      0    /* Chase permanent */
};

static const uint16_t sc_durations_L5[SC_NUM_PHASES] = {
    375,   /* Scatter 5s at 75fps */
   1500,   /* Chase 20s */
    375,   /* Scatter 5s */
   1500,   /* Chase 20s */
    375,   /* Scatter 5s */
  65535,   /* Chase ~874s (uint16_t max, arcade ~1037s) */
      1,   /* Scatter 1/75s (reversal only) */
      0    /* Chase permanent */
};

/* Pointer to current level's duration table */
static const uint16_t *cur_sc_durations;

/* 0 = scatter phase, 1 = chase phase */
static const uint8_t sc_phase_is_chase[SC_NUM_PHASES] = {
    0, 1, 0, 1, 0, 1, 0, 1
};

/* Global scatter/chase state */
uint8_t  sc_phase_index;
uint16_t sc_phase_timer;
uint8_t  sc_is_chase;

/* Ghost house state */
uint16_t ghost_house_force_timer;
uint8_t  global_dot_counter_active;
uint8_t  global_dot_counter;
uint16_t fright_timer;
uint8_t  num_ghosts_eaten;
uint8_t  ghost_eaten_freeze;
uint8_t  elroy_mode = 0;

/* Elroy parameters per level group (Table A.1 from Pac-Man Dossier).
 * 9 distinct combinations of dot thresholds and speeds.
 * Speeds: 80%=346, 85%=368, 90%=390, 95%=411, 100%=433, 105%=455. */
typedef struct {
    uint8_t  dots_elroy1;   /* dot_count threshold for Elroy 1 */
    uint8_t  dots_elroy2;   /* dot_count threshold for Elroy 2 */
    uint16_t speed_elroy1;  /* fixed-point 8.8 speed for Elroy 1 */
    uint16_t speed_elroy2;  /* fixed-point 8.8 speed for Elroy 2 */
} elroy_params_t;

static const elroy_params_t elroy_table[9] = {
    {  20,  10, 346, 368 },  /* [0] L1:     E1=80%  E2=85%  */
    {  30,  15, 390, 411 },  /* [1] L2:     E1=90%  E2=95%  */
    {  40,  20, 390, 411 },  /* [2] L3-4:   E1=90%  E2=95%  */
    {  40,  20, 433, 455 },  /* [3] L5:     E1=100% E2=105% */
    {  50,  25, 433, 455 },  /* [4] L6-8:   E1=100% E2=105% */
    {  60,  30, 433, 455 },  /* [5] L9-11:  E1=100% E2=105% */
    {  80,  40, 433, 455 },  /* [6] L12-14: E1=100% E2=105% */
    { 100,  50, 433, 455 },  /* [7] L15-18: E1=100% E2=105% */
    { 120,  60, 433, 455 },  /* [8] L19+:   E1=100% E2=105% */
};

static const elroy_params_t *cur_elroy;

/* PRNG state for frightened mode (LFSR 16-bit).
 * Reset at each level/life for reproducibility (arcade behavior). */
static uint16_t prng_state = 0x1234;

void ghosts_init(void) {
    uint8_t i;
    for (i = 0; i < NUM_GHOSTS; i++) {
        ghosts[i].tile_x = start_tile_x[i];
        ghosts[i].tile_y = start_tile_y[i];
        ghosts[i].sub_x = 6;
        ghosts[i].sub_y = 6;
        ghosts[i].dir_current = start_dir[i];
        ghosts[i].dir_next = start_dir[i];
        ghosts[i].state = start_state[i];
        ghosts[i].type = i;
        ghosts[i].target_x = scatter_target_x[i];
        ghosts[i].target_y = scatter_target_y[i];
        ghosts[i].speed_acc = 0;
        ghosts[i].anim_tick = 0;
        ghosts[i].reversal_pending = 0;
        ghosts[i].dot_counter = 0;
        {
            uint8_t dl_idx;
            if (game_level == 0)      dl_idx = 0;
            else if (game_level == 1) dl_idx = 1;
            else                      dl_idx = 2;
            ghosts[i].dot_limit = dot_limit_table[dl_idx][i];
        }
        ghosts[i].just_eaten = 0;
        ghosts[i].eaten_seq = 0;
    }

    /* House ghosts start at tile edge (sub_x=0) matching arcade X positioning.
     * Blinky (index 0) stays at sub_x=6 as he's outside the house and needs
     * tile-center alignment for pathfinding. */
    for (i = 1; i < NUM_GHOSTS; i++) {
        ghosts[i].sub_x = 0;
    }

    /* Initialize scatter/chase timer */
    /* Select scatter/chase timing table for current level */
    if (game_level == 0) {
        cur_sc_durations = sc_durations_L1;
    } else if (game_level <= 3) {
        cur_sc_durations = sc_durations_L2;
    } else {
        cur_sc_durations = sc_durations_L5;
    }
    sc_phase_index = 0;
    sc_phase_timer = cur_sc_durations[0];
    sc_is_chase = 0;

    /* Initialize ghost house state */
    ghost_house_force_timer = 0;
    global_dot_counter_active = 0;
    global_dot_counter = 0;
    fright_timer = 0;
    prng_state = 0x1234;
    num_ghosts_eaten = 0;
    ghost_eaten_freeze = 0;

    elroy_mode = 0;

    /* Set Elroy parameters for current level (Table A.1) */
    {
        uint8_t idx;
        if (game_level == 0)        idx = 0;  /* L1 */
        else if (game_level == 1)   idx = 1;  /* L2 */
        else if (game_level <= 3)   idx = 2;  /* L3-4 */
        else if (game_level == 4)   idx = 3;  /* L5 */
        else if (game_level <= 7)   idx = 4;  /* L6-8 */
        else if (game_level <= 10)  idx = 5;  /* L9-11 */
        else if (game_level <= 13)  idx = 6;  /* L12-14 */
        else if (game_level <= 17)  idx = 7;  /* L15-18 */
        else                        idx = 8;  /* L19+ */
        cur_elroy = &elroy_table[idx];
    }
}

/* Update Elroy mode based on remaining dots.
 * Only applies to Blinky (ghost 0) in scatter/chase state. */
static void elroy_update(void) {
    ghost_t *blinky = &ghosts[GHOST_BLINKY];

    /* Elroy only active when Blinky is in scatter or chase */
    if (blinky->state != GHOST_STATE_SCATTER &&
        blinky->state != GHOST_STATE_CHASE) {
        return;
    }

    if (dot_count <= cur_elroy->dots_elroy2) {
        elroy_mode = 2;
    } else if (dot_count <= cur_elroy->dots_elroy1) {
        elroy_mode = 1;
    } else {
        elroy_mode = 0;
    }
}

/* Advance scatter/chase timer. Called once per frame.
 * When phase transitions occur, sets reversal_pending on all ghosts
 * and updates their state and target. */
static void scatter_chase_tick(void) {
    uint8_t gi;

    /* Pause scatter/chase timer while any ghost is frightened */
    if (fright_timer > 0) return;

    /* Phase 7 is permanent chase — no more transitions */
    if (sc_phase_index >= SC_NUM_PHASES - 1) return;

    /* Tick down */
    if (sc_phase_timer > 0) {
        sc_phase_timer--;
        return;
    }

    /* Phase transition */
    sc_phase_index++;
    sc_phase_timer = cur_sc_durations[sc_phase_index];
    sc_is_chase = sc_phase_is_chase[sc_phase_index];

    /* Trigger global reversal: all active ghosts must reverse */
    for (gi = 0; gi < NUM_GHOSTS; gi++) {
        if (ghosts[gi].state == GHOST_STATE_SCATTER ||
            ghosts[gi].state == GHOST_STATE_CHASE) {
            /* Set new state */
            ghosts[gi].state = sc_is_chase ? GHOST_STATE_CHASE : GHOST_STATE_SCATTER;
            /* Signal reversal — will be applied at next tile center */
            ghosts[gi].reversal_pending = 1;
        }
    }
}

/* Check if ghost can walk into adjacent tile in given direction.
 * Ghosts in scatter/chase cannot enter ghost house or walk through walls.
 * Ghosts CAN use the tunnel (row 14 wraps). */
static uint8_t ghost_can_walk(uint8_t tile_x, uint8_t tile_y, uint8_t dir, uint8_t state) {
    int8_t nx = (int8_t)tile_x + dir_dx[dir];
    int8_t ny = (int8_t)tile_y + dir_dy[dir];
    uint8_t cell;

    /* Tunnel wrap */
    if (nx < 0) nx = 27;
    if (nx > 27) nx = 0;
    if (ny < 0 || ny > 30) return 0;

    cell = maze_logic[ny][nx];

    /* Must be walkable */
    if (!(cell & CELL_WALKABLE)) return 0;

    /* Ghosts in scatter/chase/frightened cannot re-enter the ghost house */
    if ((state == GHOST_STATE_SCATTER || state == GHOST_STATE_CHASE ||
         state == GHOST_STATE_FRIGHTENED) &&
        (cell & CELL_GHOST_HOUSE)) return 0;

    return 1;
}

/* Red zones: two areas where ghosts cannot turn upward.
 * These are at the tiles ABOVE the ghost house entrance, on two specific rows.
 * The lookahead tile position is checked (not the ghost's current position). */
static uint8_t is_redzone(uint8_t tile_x, uint8_t tile_y) {
    if (tile_x >= 11 && tile_x <= 16) {
        if (tile_y == 14 || tile_y == 26) return 1;
    }
    return 0;
}

/* Choose direction at intersection using distance²-based targeting.
 * Called with the LOOKAHEAD tile position (one tile ahead of ghost).
 * Uses arcade tie-break order: UP, LEFT, DOWN, RIGHT.
 * Returns the best direction to take from lookahead_x/lookahead_y. */
static uint8_t ghost_choose_direction(uint8_t lookahead_x, uint8_t lookahead_y,
                                       uint8_t current_dir, uint8_t state,
                                       uint8_t target_x, uint8_t target_y) {
    static const uint8_t priority[4] = { DIR_UP, DIR_LEFT, DIR_DOWN, DIR_RIGHT };
    uint8_t reverse = DIR_OPPOSITE(current_dir);
    uint16_t min_dist = 0xFFFF;
    uint8_t best_dir = current_dir;
    uint8_t i;
    uint8_t redzone = is_redzone(lookahead_x, lookahead_y);

    for (i = 0; i < 4; i++) {
        uint8_t dir = priority[i];
        int8_t test_x, test_y;
        uint8_t adx, ady;
        uint16_t dist;

        if (dir == reverse) continue;
        if (redzone && dir == DIR_UP && state != GHOST_STATE_EYES) continue;

        test_x = (int8_t)lookahead_x + dir_dx[dir];
        test_y = (int8_t)lookahead_y + dir_dy[dir];

        if (test_x < 0) test_x = 27;
        if (test_x > 27) test_x = 0;
        if (test_y < 0 || test_y > 30) continue;

        if (!(maze_logic[test_y][test_x] & CELL_WALKABLE)) continue;

        if ((state == GHOST_STATE_SCATTER || state == GHOST_STATE_CHASE ||
             state == GHOST_STATE_FRIGHTENED) &&
            (maze_logic[test_y][test_x] & CELL_GHOST_HOUSE)) continue;

        /* Distance² using uint8_t abs values → 8-bit multiply (much faster on Z80) */
        adx = (uint8_t)(((uint8_t)test_x >= target_x) ? ((uint8_t)test_x - target_x) : (target_x - (uint8_t)test_x));
        ady = (uint8_t)(((uint8_t)test_y >= target_y) ? ((uint8_t)test_y - target_y) : (target_y - (uint8_t)test_y));
        dist = (uint16_t)adx * adx + (uint16_t)ady * ady;

        if (dist < min_dist) {
            min_dist = dist;
            best_dir = dir;
        }
    }
    return best_dir;
}

/* Check ghost house exit conditions. Called once per frame from ghosts_update().
 * Checks force timer, global dot counter, and personal dot counters.
 * Transitions ghosts from HOUSE to LEAVEHOUSE when conditions are met. */
static void ghost_house_tick(void) {
    uint8_t i;

    /* Increment force timer */
    ghost_house_force_timer++;

    for (i = 0; i < NUM_GHOSTS; i++) {
        if (ghosts[i].state != GHOST_STATE_HOUSE) continue;

        /* --- Check exit conditions in priority order --- */

        /* 1. Force timer: no dots eaten for 4 seconds */
        if (ghost_house_force_timer >= FORCE_LEAVE_FRAMES) {
            ghosts[i].state = GHOST_STATE_LEAVEHOUSE;
            ghost_house_force_timer = 0;  /* Reset for next ghost */
            break;  /* Only one ghost exits per trigger */
        }

        /* 2. Global dot counter (active after life lost) */
        if (global_dot_counter_active) {
            uint8_t should_leave = 0;
            if (ghosts[i].type == GHOST_PINKY && global_dot_counter >= GLOBAL_DOT_PINKY) {
                should_leave = 1;
            } else if (ghosts[i].type == GHOST_INKY && global_dot_counter >= GLOBAL_DOT_INKY) {
                should_leave = 1;
            } else if (ghosts[i].type == GHOST_CLYDE && global_dot_counter >= GLOBAL_DOT_CLYDE) {
                should_leave = 1;
                /* Clyde reaching threshold deactivates global counter */
                global_dot_counter_active = 0;
            }
            if (should_leave) {
                ghosts[i].state = GHOST_STATE_LEAVEHOUSE;
                break;
            }
            /* In global mode, skip personal counter check */
            continue;
        }

        /* 3. Personal dot counter reached limit */
        if (ghosts[i].dot_counter >= ghosts[i].dot_limit) {
            ghosts[i].state = GHOST_STATE_LEAVEHOUSE;
            break;
        }
    }
}

/* Tick down frightened timer. When expired, return ghosts to scatter/chase.
 * No reversal when exiting frightened (arcade behavior).
 * No need to fix dir_next — it was always computed by standard look-ahead
 * during FRIGHTENED mode (using random targets). */
static void frightened_tick(void) {
    uint8_t i;

    if (fright_timer == 0) return;

    fright_timer--;
    if (fright_timer > 0) return;

    /* Frightened expired: return all FRIGHTENED ghosts to current scatter/chase */
    for (i = 0; i < NUM_GHOSTS; i++) {
        if (ghosts[i].state == GHOST_STATE_FRIGHTENED) {
            ghosts[i].state = sc_is_chase ? GHOST_STATE_CHASE : GHOST_STATE_SCATTER;
            /* NO reversal_pending — arcade does not reverse on fright end */
            /* dir_next is already valid from the last look-ahead computation */
        }
    }
}

/* Generate a pseudo-random direction (0-3) using 16-bit LFSR.
 * Taps at bits 0, 2, 3, 5 — matches arcade PRNG. */
static uint8_t prng_direction(void) {
    uint16_t bit = ((prng_state >> 0) ^ (prng_state >> 2) ^
                    (prng_state >> 3) ^ (prng_state >> 5)) & 1;
    prng_state = (prng_state >> 1) | (bit << 15);
    return (uint8_t)(prng_state & 0x03);
}

/* Update ghost target tile based on current state.
 * SCATTER: fixed corner. CHASE: per-ghost targeting.
 * FRIGHTENED: random target (same pathfinding, random destination).
 * Called once per frame for each active ghost. */
static void ghost_update_target(ghost_t *g) {
    /* EYES: target is the ghost house entrance (above the door) */
    if (g->state == GHOST_STATE_EYES) {
        g->target_x = HOUSE_EXIT_TILE_X;   /* 14 */
        g->target_y = HOUSE_EXIT_TILE_Y;   /* 11 */
        return;
    }

    /* Scatter: fixed corner target (Elroy: Blinky ignores scatter) */
    if (g->state == GHOST_STATE_SCATTER) {
        if (g->type == GHOST_BLINKY && elroy_mode > 0) {
            g->target_x = zpac.tile_x;
            g->target_y = zpac.tile_y;
        } else {
            g->target_x = scatter_target_x[g->type];
            g->target_y = scatter_target_y[g->type];
        }
        return;
    }

    /* Frightened: random target.
     * We set a random target and let the standard distance²-based pathfinding
     * pick the direction. This keeps look-ahead working identically in all states. */
    if (g->state == GHOST_STATE_FRIGHTENED) {
        g->target_x = prng_direction() * 7;  /* 0..21 range (4 values * 7) */
        g->target_y = prng_direction() * 8;  /* 0..24 range (4 values * 8) */
        return;
    }

    /* Chase: unique targeting per ghost type */
    if (g->state != GHOST_STATE_CHASE) return;

    switch (g->type) {

    case GHOST_BLINKY:
        /* Direct chase: target = ZPac's current tile */
        g->target_x = zpac.tile_x;
        g->target_y = zpac.tile_y;
        break;

    case GHOST_PINKY: {
        /* 4 tiles ahead of ZPac in his facing direction.
         * Arcade overflow bug: when ZPac faces UP, the target
         * is ALSO shifted 4 tiles LEFT (original Z80 code applied
         * the offset to both X and Y via a single pointer increment). */
        int8_t tx = (int8_t)zpac.tile_x + dir_dx[zpac.dir_current] * 4;
        int8_t ty = (int8_t)zpac.tile_y + dir_dy[zpac.dir_current] * 4;
        if (zpac.dir_current == DIR_UP) {
            tx -= 4;  /* Overflow bug replica */
        }
        if (tx < 0) tx = 0;
        if (tx > 27) tx = 27;
        if (ty < 0) ty = 0;
        if (ty > 30) ty = 30;
        g->target_x = (uint8_t)tx;
        g->target_y = (uint8_t)ty;
        break;
    }

    case GHOST_INKY: {
        /* Triangulation through a point 2 tiles ahead of ZPac.
         * P = zpac_pos + 2 * zpac_dir
         * target = 2*P - blinky_pos  (= P + (P - blinky_pos))
         * Same overflow bug: if ZPac faces UP, P is also 2 tiles LEFT. */
        int8_t px = (int8_t)zpac.tile_x + dir_dx[zpac.dir_current] * 2;
        int8_t py = (int8_t)zpac.tile_y + dir_dy[zpac.dir_current] * 2;
        int8_t bx = (int8_t)ghosts[GHOST_BLINKY].tile_x;
        int8_t by = (int8_t)ghosts[GHOST_BLINKY].tile_y;
        int8_t tx, ty;
        if (zpac.dir_current == DIR_UP) {
            px -= 2;  /* Overflow bug replica */
        }
        tx = px + (px - bx);
        ty = py + (py - by);
        if (tx < 0) tx = 0;
        if (tx > 27) tx = 27;
        if (ty < 0) ty = 0;
        if (ty > 30) ty = 30;
        g->target_x = (uint8_t)tx;
        g->target_y = (uint8_t)ty;
        break;
    }

    case GHOST_CLYDE: {
        /* Proximity switch: if far from ZPac (dist² > 64, i.e. > 8 tiles),
         * chase directly like Blinky. If close, retreat to scatter corner. */
        uint8_t adx, ady;
        uint16_t dist_sq;
        adx = (g->tile_x >= zpac.tile_x) ?
              (g->tile_x - zpac.tile_x) : (zpac.tile_x - g->tile_x);
        ady = (g->tile_y >= zpac.tile_y) ?
              (g->tile_y - zpac.tile_y) : (zpac.tile_y - g->tile_y);
        dist_sq = (uint16_t)adx * adx + (uint16_t)ady * ady;
        if (dist_sq > 64) {
            g->target_x = zpac.tile_x;
            g->target_y = zpac.tile_y;
        } else {
            g->target_x = scatter_target_x[GHOST_CLYDE];
            g->target_y = scatter_target_y[GHOST_CLYDE];
        }
        break;
    }

    default:
        break;
    }
}

void ghosts_enter_frightened(void) {
    uint8_t i;
    num_ghosts_eaten = 0;
    fright_timer = get_fright_duration();

    if (fright_timer == 0) {
        /* No frightened mode at this level, but arcade still reverses */
        for (i = 0; i < NUM_GHOSTS; i++) {
            if (ghosts[i].state == GHOST_STATE_SCATTER ||
                ghosts[i].state == GHOST_STATE_CHASE) {
                ghosts[i].reversal_pending = 1;
            }
        }
        return;
    }

    for (i = 0; i < NUM_GHOSTS; i++) {
        if (ghosts[i].state == GHOST_STATE_SCATTER ||
            ghosts[i].state == GHOST_STATE_CHASE) {
            ghosts[i].state = GHOST_STATE_FRIGHTENED;
            ghosts[i].reversal_pending = 1;
        }
        /* Ghosts in HOUSE, LEAVEHOUSE, EYES, ENTERHOUSE: NOT affected */
    }
}

void ghosts_update(void) {
    uint8_t gi;

    /* Tick down ghost eaten freeze (global) */
    if (ghost_eaten_freeze > 0) {
        ghost_eaten_freeze--;
        /* During freeze: tick down per-ghost just_eaten timers but don't move */
        for (gi = 0; gi < NUM_GHOSTS; gi++) {
            if (ghosts[gi].just_eaten > 0) {
                ghosts[gi].just_eaten--;
            }
        }
        return;  /* All ghosts frozen during eat-ghost pause */
    }

    scatter_chase_tick();
    elroy_update();
    frightened_tick();
    ghost_house_tick();
    for (gi = 0; gi < NUM_GHOSTS; gi++) {
        ghost_t *g = &ghosts[gi];
        uint8_t steps, s;

        /* Ghost house bouncing: vertical oscillation in slot */
        if (g->state == GHOST_STATE_HOUSE) {
            uint8_t h_steps, h_s;

            /* Speed accumulator at house speed (roughly half normal) */
            g->speed_acc += SPD_GHOST_HOUSE;
            h_steps = (uint8_t)(g->speed_acc >> 8);
            g->speed_acc &= 0xFF;

            for (h_s = 0; h_s < h_steps; h_s++) {
                /* Move 1 pixel in current vertical direction */
                if (g->dir_current == DIR_DOWN) {
                    g->sub_y++;
                    if (g->sub_y >= 9) {
                        g->sub_y = 9;
                        g->dir_current = DIR_UP;
                    }
                } else {
                    /* DIR_UP or any other — treat as UP */
                    if (g->sub_y > 0) {
                        g->sub_y--;
                    }
                    if (g->sub_y <= 3) {
                        g->sub_y = 3;
                        g->dir_current = DIR_DOWN;
                    }
                }
            }

            g->anim_tick++;
            continue;
        }

        /* LEAVEHOUSE: navigate from house slot to exit above door */
        if (g->state == GHOST_STATE_LEAVEHOUSE) {
            uint8_t h_steps, h_s;

            g->speed_acc += SPD_GHOST_HOUSE;
            h_steps = (uint8_t)(g->speed_acc >> 8);
            g->speed_acc &= 0xFF;

            for (h_s = 0; h_s < h_steps; h_s++) {
                uint16_t cur_px;
                int8_t new_sub_x, new_sub_y;
                int8_t dx, dy;

                /* Check if reached exit point: tile (14,11) sub (6,6) */
                if (g->tile_x == HOUSE_EXIT_TILE_X && g->sub_x == 6 &&
                    g->tile_y == HOUSE_EXIT_TILE_Y && g->sub_y == 6) {
                    /* Transition to active maze movement */
                    g->state = sc_is_chase ? GHOST_STATE_CHASE : GHOST_STATE_SCATTER;
                    g->dir_current = DIR_LEFT;
                    g->dir_next = DIR_LEFT;
                    g->target_x = scatter_target_x[g->type];
                    g->target_y = scatter_target_y[g->type];
                    break;
                }

                /* Decide direction based on current position */
                /* 174 = center X pixel = (14 << 3) + (14 << 2) + 6 */
                cur_px = ((uint16_t)g->tile_x << 3) + ((uint16_t)g->tile_x << 2) + g->sub_x;

                if (cur_px == 174) {
                    /* At center X column: move UP toward exit */
                    g->dir_current = DIR_UP;
                } else if (g->sub_y != 6) {
                    /* Not at center X yet: first center Y in slot */
                    g->dir_current = (g->sub_y > 6) ? DIR_UP : DIR_DOWN;
                } else {
                    /* Y centered: move horizontally toward center X */
                    g->dir_current = (cur_px < 174) ? DIR_RIGHT : DIR_LEFT;
                }

                /* Move 1 pixel - forced movement, ignores walls */
                dx = dir_dx[g->dir_current];
                dy = dir_dy[g->dir_current];
                new_sub_x = (int8_t)g->sub_x + dx;
                new_sub_y = (int8_t)g->sub_y + dy;

                if (new_sub_x >= 12) { new_sub_x -= 12; g->tile_x++; }
                else if (new_sub_x < 0) { new_sub_x += 12; g->tile_x--; }
                if (new_sub_y >= 12) { new_sub_y -= 12; g->tile_y++; }
                else if (new_sub_y < 0) { new_sub_y += 12; g->tile_y--; }

                g->sub_x = (uint8_t)new_sub_x;
                g->sub_y = (uint8_t)new_sub_y;
            }

            g->anim_tick++;
            continue;
        }

        /* EYES: navigate back to ghost house at high speed.
         * Uses standard pathfinding toward (14, 11).
         * EYES ghosts can enter the ghost house (they ignore the CELL_GHOST_HOUSE block). */
        if (g->state == GHOST_STATE_EYES) {
            uint8_t e_steps, e_s;

            ghost_update_target(g);

            g->speed_acc += SPD_GHOST_EYES_L1;
            e_steps = (uint8_t)(g->speed_acc >> 8);
            g->speed_acc &= 0xFF;

            for (e_s = 0; e_s < e_steps; e_s++) {
                int8_t dx, dy;
                int8_t new_sub_x, new_sub_y;

                /* At tile center: check arrival or decide direction */
                if (g->sub_x == 6 && g->sub_y == 6) {
                    if (e_s > 0) break;

                    /* Check if arrived at house entrance */
                    if (g->tile_x == HOUSE_EXIT_TILE_X &&
                        g->tile_y == HOUSE_EXIT_TILE_Y) {
                        g->state = GHOST_STATE_ENTERHOUSE;
                        g->dir_current = DIR_DOWN;
                        g->speed_acc = 0;
                        break;
                    }

                    /* Apply look-ahead direction */
                    g->dir_current = g->dir_next;

                    /* Compute next look-ahead */
                    {
                        int8_t look_x = (int8_t)g->tile_x + dir_dx[g->dir_current];
                        int8_t look_y = (int8_t)g->tile_y + dir_dy[g->dir_current];
                        if (look_x < 0) look_x = 27;
                        if (look_x > 27) look_x = 0;
                        if (look_y >= 0 && look_y <= 30) {
                            g->dir_next = ghost_choose_direction(
                                (uint8_t)look_x, (uint8_t)look_y,
                                g->dir_current, g->state,
                                g->target_x, g->target_y);
                        }
                    }
                }

                /* Move 1 pixel */
                dx = dir_dx[g->dir_current];
                dy = dir_dy[g->dir_current];
                new_sub_x = (int8_t)g->sub_x + dx;
                new_sub_y = (int8_t)g->sub_y + dy;

                if (new_sub_x >= 12) { new_sub_x -= 12; if (g->tile_x >= 27) g->tile_x = 0; else g->tile_x++; }
                else if (new_sub_x < 0) { new_sub_x += 12; if (g->tile_x == 0) g->tile_x = 27; else g->tile_x--; }
                if (new_sub_y >= 12) { new_sub_y -= 12; g->tile_y++; }
                else if (new_sub_y < 0) { new_sub_y += 12; g->tile_y--; }

                g->sub_x = (uint8_t)new_sub_x;
                g->sub_y = (uint8_t)new_sub_y;
            }

            g->anim_tick++;
            continue;
        }

        /* ENTERHOUSE: forced movement down into ghost house slot */
        if (g->state == GHOST_STATE_ENTERHOUSE) {
            uint8_t h_steps, h_s;

            g->speed_acc += SPD_GHOST_EYES_L1;  /* Same fast speed as EYES */
            h_steps = (uint8_t)(g->speed_acc >> 8);
            g->speed_acc &= 0xFF;

            for (h_s = 0; h_s < h_steps; h_s++) {
                int8_t new_sub_x, new_sub_y;
                int8_t dx, dy;
                uint16_t cur_px;

                /* Check if reached target slot */
                if (g->tile_x == house_target_x[g->type] &&
                    g->tile_y == house_target_y[g->type] &&
                    g->sub_x == 6 && g->sub_y == 6) {
                    /* Reached home slot: immediately leave house again */
                    g->state = GHOST_STATE_LEAVEHOUSE;
                    g->dir_current = DIR_UP;
                    g->speed_acc = 0;
                    break;
                }

                /* Navigate toward slot: first go down, then horizontal */
                cur_px = ((uint16_t)g->tile_x << 3) + ((uint16_t)g->tile_x << 2) + g->sub_x;
                {
                    uint16_t tgt_px = ((uint16_t)house_target_x[g->type] << 3) +
                                      ((uint16_t)house_target_x[g->type] << 2) + 6;

                    if (g->tile_y < house_target_y[g->type] ||
                        (g->tile_y == house_target_y[g->type] && g->sub_y < 6)) {
                        /* Still above target row: go DOWN */
                        g->dir_current = DIR_DOWN;
                    } else if (cur_px != tgt_px) {
                        /* At target row but wrong column: go horizontal */
                        g->dir_current = (cur_px < tgt_px) ? DIR_RIGHT : DIR_LEFT;
                    } else if (g->sub_y != 6) {
                        /* At target column: center vertically */
                        g->dir_current = (g->sub_y > 6) ? DIR_UP : DIR_DOWN;
                    }
                }

                /* Forced move (ignores walls — inside house) */
                dx = dir_dx[g->dir_current];
                dy = dir_dy[g->dir_current];
                new_sub_x = (int8_t)g->sub_x + dx;
                new_sub_y = (int8_t)g->sub_y + dy;

                if (new_sub_x >= 12) { new_sub_x -= 12; g->tile_x++; }
                else if (new_sub_x < 0) { new_sub_x += 12; g->tile_x--; }
                if (new_sub_y >= 12) { new_sub_y -= 12; g->tile_y++; }
                else if (new_sub_y < 0) { new_sub_y += 12; g->tile_y--; }

                g->sub_x = (uint8_t)new_sub_x;
                g->sub_y = (uint8_t)new_sub_y;
            }

            g->anim_tick++;
            continue;
        }

        /* Update target for ALL active ghosts (scatter/chase/frightened).
         * Frightened ghosts now get a random target. */
        ghost_update_target(g);

        /* 1. Speed accumulator — frightened ghosts are slower */
        {
            uint16_t spd;
            if (g->state == GHOST_STATE_FRIGHTENED) {
                spd = cur_spd.ghost_fright;
            } else {
                /* Check tunnel: ghosts slow down in the tunnel */
                uint8_t in_tunnel = maze_logic[g->tile_y][g->tile_x] & CELL_TUNNEL;
                if (in_tunnel) {
                    spd = cur_spd.ghost_tunnel;
                } else if (g->type == GHOST_BLINKY && elroy_mode == 2) {
                    spd = cur_elroy->speed_elroy2;
                } else if (g->type == GHOST_BLINKY && elroy_mode == 1) {
                    spd = cur_elroy->speed_elroy1;
                } else {
                    spd = cur_spd.ghost_normal;
                }
            }
            g->speed_acc += spd;
        }
        steps = (uint8_t)(g->speed_acc >> 8);
        g->speed_acc &= 0xFF;

        /* 2. Move pixel by pixel */
        for (s = 0; s < steps; s++) {
            int8_t dx, dy;
            int8_t new_sub_x, new_sub_y;

            /* 2a. At tile center: decide direction.
             * CRITICAL FIX: if arriving at center during multi-pixel move (s > 0),
             * break FIRST without processing. Direction will be processed next
             * frame when s == 0. This ensures exactly ONE processing per tile center,
             * matching the arcade behavior. */
            if (g->sub_x == 6 && g->sub_y == 6) {
                /* Break on arrival — don't process direction twice */
                if (s > 0) break;

                /* Apply reversal if pending (from scatter↔chase transition
                 * or entering frightened mode) */
                if (g->reversal_pending) {
                    g->dir_current = DIR_OPPOSITE(g->dir_current);
                    g->reversal_pending = 0;
                } else {
                    /* ALL states: apply pre-computed look-ahead direction.
                     * FRIGHTENED ghosts also use look-ahead now (toward random target).
                     * No more separate ghost_choose_direction_frightened(). */
                    g->dir_current = g->dir_next;
                }

                /* Compute dir_next for the NEXT tile center (look-ahead).
                 * This is done for ALL states including FRIGHTENED.
                 * Same pathfinding logic, different target per state. */
                {
                    int8_t look_x = (int8_t)g->tile_x + dir_dx[g->dir_current];
                    int8_t look_y = (int8_t)g->tile_y + dir_dy[g->dir_current];
                    if (look_x < 0) look_x = 27;
                    if (look_x > 27) look_x = 0;
                    if (look_y >= 0 && look_y <= 30) {
                        g->dir_next = ghost_choose_direction(
                            (uint8_t)look_x, (uint8_t)look_y,
                            g->dir_current, g->state,
                            g->target_x, g->target_y);
                    }
                }

                /* Wall safety check — should be extremely rare with correct
                 * look-ahead, but protects against edge cases. */
                if (!ghost_can_walk(g->tile_x, g->tile_y, g->dir_current, g->state)) {
                    static const uint8_t fb_pri[4] = { DIR_UP, DIR_LEFT, DIR_DOWN, DIR_RIGHT };
                    uint8_t reverse = DIR_OPPOSITE(g->dir_current);
                    uint8_t fi;
                    uint8_t found = 0;
                    for (fi = 0; fi < 4; fi++) {
                        if (fb_pri[fi] == reverse) continue;
                        if (ghost_can_walk(g->tile_x, g->tile_y, fb_pri[fi], g->state)) {
                            g->dir_current = fb_pri[fi];
                            found = 1;
                            break;
                        }
                    }
                    if (!found) {
                        if (ghost_can_walk(g->tile_x, g->tile_y, reverse, g->state)) {
                            g->dir_current = reverse;
                            found = 1;
                        }
                    }
                    if (!found) break;

                    /* Recompute look-ahead for recovery direction */
                    {
                        int8_t look_x = (int8_t)g->tile_x + dir_dx[g->dir_current];
                        int8_t look_y = (int8_t)g->tile_y + dir_dy[g->dir_current];
                        if (look_x < 0) look_x = 27;
                        if (look_x > 27) look_x = 0;
                        if (look_y >= 0 && look_y <= 30) {
                            g->dir_next = ghost_choose_direction(
                                (uint8_t)look_x, (uint8_t)look_y,
                                g->dir_current, g->state,
                                g->target_x, g->target_y);
                        }
                    }
                }
            }

            /* 2b. Move 1 pixel in current direction */
            dx = dir_dx[g->dir_current];
            dy = dir_dy[g->dir_current];
            new_sub_x = (int8_t)g->sub_x + dx;
            new_sub_y = (int8_t)g->sub_y + dy;

            /* 2c. Handle tile boundary crossing */
            if (new_sub_x >= 12) {
                new_sub_x -= 12;
                if (g->tile_x >= 27) g->tile_x = 0;
                else g->tile_x++;
            } else if (new_sub_x < 0) {
                new_sub_x += 12;
                if (g->tile_x == 0) g->tile_x = 27;
                else g->tile_x--;
            }

            if (new_sub_y >= 12) {
                new_sub_y -= 12;
                g->tile_y++;
            } else if (new_sub_y < 0) {
                new_sub_y += 12;
                g->tile_y--;
            }

            g->sub_x = (uint8_t)new_sub_x;
            g->sub_y = (uint8_t)new_sub_y;
        }

        /* 3. Animation tick */
        g->anim_tick++;
    }
}

/* Sprite offset calibration (same as ZPac) */
#define GHOST_SPR_OFF_X  1
#define GHOST_SPR_OFF_Y  1

/* Ghost tile table: [direction][frame][4 quadrants TL,TR,BL,BR]
 * Direction order: RIGHT=0, DOWN=1, LEFT=2, UP=3
 * Frame order: F0=0, F1=1 */
static const uint16_t ghost_tiles[4][2][4] = {
    /* DIR_RIGHT */
    { { SPR_GHOST_RIGHT_F0_TL, SPR_GHOST_RIGHT_F0_TR, SPR_GHOST_RIGHT_F0_BL, SPR_GHOST_RIGHT_F0_BR },
      { SPR_GHOST_RIGHT_F1_TL, SPR_GHOST_RIGHT_F1_TR, SPR_GHOST_RIGHT_F1_BL, SPR_GHOST_RIGHT_F1_BR } },
    /* DIR_DOWN */
    { { SPR_GHOST_DOWN_F0_TL, SPR_GHOST_DOWN_F0_TR, SPR_GHOST_DOWN_F0_BL, SPR_GHOST_DOWN_F0_BR },
      { SPR_GHOST_DOWN_F1_TL, SPR_GHOST_DOWN_F1_TR, SPR_GHOST_DOWN_F1_BL, SPR_GHOST_DOWN_F1_BR } },
    /* DIR_LEFT */
    { { SPR_GHOST_LEFT_F0_TL, SPR_GHOST_LEFT_F0_TR, SPR_GHOST_LEFT_F0_BL, SPR_GHOST_LEFT_F0_BR },
      { SPR_GHOST_LEFT_F1_TL, SPR_GHOST_LEFT_F1_TR, SPR_GHOST_LEFT_F1_BL, SPR_GHOST_LEFT_F1_BR } },
    /* DIR_UP */
    { { SPR_GHOST_UP_F0_TL, SPR_GHOST_UP_F0_TR, SPR_GHOST_UP_F0_BL, SPR_GHOST_UP_F0_BR },
      { SPR_GHOST_UP_F1_TL, SPR_GHOST_UP_F1_TR, SPR_GHOST_UP_F1_BL, SPR_GHOST_UP_F1_BR } }
};

/* Palette group per ghost type (from zpac_maze_data.h) */
static const uint8_t ghost_palette[NUM_GHOSTS] = {
    PAL_BLINKY, PAL_PINKY, PAL_INKY, PAL_CLYDE
};

static uint16_t ghost_tile_sub_to_pixel(uint8_t tile, uint8_t sub) {
    return ((uint16_t)tile << 3) + ((uint16_t)tile << 2) + sub;
}

void ghosts_render(void) {
    uint8_t gi;
    for (gi = 0; gi < NUM_GHOSTS; gi++) {
        ghost_t *g = &ghosts[gi];
        uint16_t px = ghost_tile_sub_to_pixel(g->tile_x, g->sub_x);
        uint16_t py = ghost_tile_sub_to_pixel(g->tile_y, g->sub_y);
        uint16_t sx = 144 + px + GHOST_SPR_OFF_X;
        uint16_t sy =  48 + py + GHOST_SPR_OFF_Y;

        uint8_t frame = (g->anim_tick >> 3) & 1;
        const uint16_t *tiles;
        uint8_t pal;
        uint8_t base_idx = GHOST_SPR_BASE(g->type);

        /* During eat-freeze: show score popup (200/400/800/1600)
         * using 2 tiles side-by-side (32x16 area) with 5x7 font.
         * Uses sprite slots 0-1, hides slots 2-3. */
        if (g->just_eaten > 0) {
            static const uint16_t score_tile_pairs[4][2] = {
                { SPR_SCORE_200_L,  SPR_SCORE_200_R  },
                { SPR_SCORE_400_L,  SPR_SCORE_400_R  },
                { SPR_SCORE_800_L,  SPR_SCORE_800_R  },
                { SPR_SCORE_1600_L, SPR_SCORE_1600_R }
            };
            uint8_t seq_idx;
            uint8_t qi;

            /* eaten_seq is 1-4, array index is 0-3 */
            seq_idx = g->eaten_seq - 1;
            if (seq_idx > 3) seq_idx = 0;

            /* Sprite 0: left tile, Sprite 1: right tile (side by side) */
            for (qi = 0; qi < 2; qi++) {
                uint16_t tile_id = score_tile_pairs[seq_idx][qi];
                gfx_sprite spr = { 0 };
                spr.x = sx + (uint16_t)(qi << 4);  /* qi*16: 0 or 16 */
                spr.y = sy + 8;   /* center vertically in 32px ghost area */
                spr.tile = tile_id & 0xFF;
                spr.flags = (uint8_t)(PAL_GHOST_SCORE << 4);
                if (tile_id >= 256) spr.flags |= 0x01;
                gfx_sprite_render(&vctx, base_idx + qi, &spr);
            }

            /* Hide remaining 2 sprites off-screen */
            for (qi = 2; qi < 4; qi++) {
                gfx_sprite spr = { 0 };
                spr.x = 0;
                spr.y = 480;
                gfx_sprite_render(&vctx, base_idx + qi, &spr);
            }
            continue;
        }

        if (g->state == GHOST_STATE_EYES ||
            g->state == GHOST_STATE_ENTERHOUSE) {
            /* Eyes only: use normal ghost F0 tile for current direction
             * with PAL_EYES — only the eyes are visible (rest is transparent) */
            tiles = ghost_tiles[g->dir_current][0];  /* always frame 0 */
            pal = PAL_EYES;
        } else if (g->state == GHOST_STATE_FRIGHTENED) {
            static const uint16_t fright_tiles[2][4] = {
                { SPR_FRIGHT_F0_TL, SPR_FRIGHT_F0_TR, SPR_FRIGHT_F0_BL, SPR_FRIGHT_F0_BR },
                { SPR_FRIGHT_F1_TL, SPR_FRIGHT_F1_TR, SPR_FRIGHT_F1_BL, SPR_FRIGHT_F1_BR }
            };
            tiles = fright_tiles[frame];

            /* Flash: alternate blue/white in the last N frames (per-level, Table A.1) */
            if (fright_timer <= get_fright_flash_frames() && fright_timer > 0) {
                pal = ((fright_timer >> 3) & 1) ? PAL_FRIGHTENED : PAL_FRIGHT_BLINK;
            } else {
                pal = PAL_FRIGHTENED;
            }
        } else {
            tiles = ghost_tiles[g->dir_current][frame];
            pal = ghost_palette[g->type];
        }

        {
            uint8_t qi;
            for (qi = 0; qi < 4; qi++) {
                gfx_sprite spr = { 0 };
                spr.x = sx + ((qi & 1) ? 16 : 0);
                spr.y = sy + ((qi & 2) ? 16 : 0);
                spr.tile = tiles[qi] & 0xFF;
                spr.flags = (pal << 4);
                if (tiles[qi] >= 256) spr.flags |= 0x01;
                gfx_sprite_render(&vctx, base_idx + qi, &spr);
            }
        }
    }
}

/* Called when ZPac eats a dot or energizer.
 * Resets force timer and increments the appropriate dot counter. */
void ghost_house_dot_eaten(void) {
    uint8_t i;

    /* Reset force timer — ZPac just ate, so no timeout */
    ghost_house_force_timer = 0;

    /* Increment dot counters */
    if (global_dot_counter_active) {
        /* Global mode: single shared counter */
        global_dot_counter++;
    } else {
        /* Personal mode: increment counter of highest-priority ghost in house
         * whose counter hasn't reached its limit yet.
         * Priority order: Blinky(0) > Pinky(1) > Inky(2) > Clyde(3)
         * (Blinky is never in HOUSE at game start, so effectively Pinky first) */
        for (i = 0; i < NUM_GHOSTS; i++) {
            if (ghosts[i].state == GHOST_STATE_HOUSE &&
                ghosts[i].dot_counter < ghosts[i].dot_limit) {
                ghosts[i].dot_counter++;
                break;  /* Only one ghost counts per dot */
            }
        }
    }
}

uint8_t ghosts_check_collision(uint8_t pac_tile_x, uint8_t pac_tile_y) {
    uint8_t gi;

    /* If frozen from a recent ghost eat, no new collisions */
    if (ghost_eaten_freeze > 0) return 0xFF;

    for (gi = 0; gi < NUM_GHOSTS; gi++) {
        if (ghosts[gi].tile_x != pac_tile_x ||
            ghosts[gi].tile_y != pac_tile_y) continue;

        if (ghosts[gi].state == GHOST_STATE_FRIGHTENED) {
            /* ZPac eats this ghost! */
            ghosts[gi].state = GHOST_STATE_EYES;
            ghosts[gi].reversal_pending = 0;
            ghosts[gi].speed_acc = 0;
            ghosts[gi].just_eaten = GHOST_EATEN_FREEZE;

            num_ghosts_eaten++;
            ghosts[gi].eaten_seq = num_ghosts_eaten;  /* 1, 2, 3, or 4 */
            /* Score: 200, 400, 800, 1600 (progressive doubling) */
            {
                uint16_t eat_score = 200;
                uint8_t m;
                for (m = 1; m < num_ghosts_eaten; m++) {
                    eat_score <<= 1;  /* double each time */
                }
                score += (uint32_t)eat_score;
            }

            ghost_eaten_freeze = GHOST_EATEN_FREEZE;
            return (uint8_t)(0x80 | gi);  /* 0x80+ = ate ghost, not death */
        }

        if (ghosts[gi].state == GHOST_STATE_SCATTER ||
            ghosts[gi].state == GHOST_STATE_CHASE) {
            return gi;  /* 0x00-0x03 = death */
        }
    }
    return 0xFF;
}
