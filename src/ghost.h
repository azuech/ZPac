#ifndef GHOST_H
#define GHOST_H

#include <stdint.h>

/* === Ghost type indices === */
#define GHOST_BLINKY  0
#define GHOST_PINKY   1
#define GHOST_INKY    2
#define GHOST_CLYDE   3
#define NUM_GHOSTS    4

/* === Ghost states === */
/* Phase 5: SCATTER, CHASE, HOUSE, LEAVEHOUSE implemented.
   FRIGHTENED/EYES deferred to Phase 6. */
#define GHOST_STATE_SCATTER     0
#define GHOST_STATE_CHASE       1
#define GHOST_STATE_FRIGHTENED  2
#define GHOST_STATE_EYES        3
#define GHOST_STATE_HOUSE       4
#define GHOST_STATE_LEAVEHOUSE  5
#define GHOST_STATE_ENTERHOUSE  6

/* === Scatter/Chase phase timing (Level 1, in frames at 75fps) === */
/* Arcade Level 1 sequence:
 * Scatter 7s -> Chase 20s -> Scatter 7s -> Chase 20s ->
 * Scatter 5s -> Chase 20s -> Scatter 5s -> Chase permanent */
#define SC_NUM_PHASES  8
/* Phase durations in frames (75fps): 7s=525, 20s=1500, 5s=375, 0=infinite */
/* Stored as extern const in ghost.c to save ROM vs repeated defines */

/* === Scatter/Chase global state (extern, defined in ghost.c) === */
extern uint8_t  sc_phase_index;    /* current phase (0-7) */
extern uint16_t sc_phase_timer;    /* frames remaining in current phase */
extern uint8_t  sc_is_chase;       /* 0=scatter, 1=chase */

/* === Ghost struct ===
 * Mirrors zpac_t layout for movement (tile+sub coordinate system).
 * Uses same speed accumulator approach (16-bit fixed-point 8.8).
 * Fields ordered for SDCC alignment on Z80. */
typedef struct {
    uint16_t speed_acc;     /* speed accumulator fixed-point 8.8 */
    uint8_t  tile_x;        /* tile X in 28x31 logic grid */
    uint8_t  tile_y;        /* tile Y in 28x31 logic grid */
    uint8_t  sub_x;         /* sub-tile X offset (0-11), center=6 */
    uint8_t  sub_y;         /* sub-tile Y offset (0-11), center=6 */
    uint8_t  dir_current;   /* current movement direction (DIR_RIGHT/DOWN/LEFT/UP) */
    uint8_t  dir_next;      /* pre-computed next direction (look-ahead) */
    uint8_t  state;         /* ghost state (GHOST_STATE_*) */
    uint8_t  type;          /* ghost type index (GHOST_BLINKY..GHOST_CLYDE) */
    uint8_t  target_x;      /* current target tile X (scatter corner or chase target) */
    uint8_t  target_y;      /* current target tile Y */
    uint8_t  anim_tick;     /* animation counter */
    uint8_t  reversal_pending; /* flag: reverse direction at next tile center */
    uint8_t  dot_counter;       /* personal dot counter (incremented by dot system) */
    uint8_t  dot_limit;         /* dot threshold to leave house (0=Pinky, 30=Inky, 60=Clyde) */
    uint8_t  just_eaten;        /* >0: recently eaten, countdown for score display */
    uint8_t  eaten_seq;         /* which sequential eat (1-4), for score popup display */
} ghost_t;

/* === Scatter target tiles ===
 * These are the fixed corner tiles each ghost aims for in scatter mode.
 * Blinky=(25,0), Pinky=(2,0), Inky=(27,30), Clyde=(0,30)
 * Note: Inky/Clyde Y clamped from original 34 to 30 (our maze is 28x31, max row=30) */
/* Declared as extern, defined in ghost.c */

/* === Hardware sprite base indices (4 sprites per ghost, 2x2 composite) === */
#define HW_SPR_GHOST_BASE  4   /* ghosts start at sprite 4 (0-3 = ZPac) */
/* Blinky=4-7, Pinky=8-11, Inky=12-15, Clyde=16-19 */
#define GHOST_SPR_BASE(type) (HW_SPR_GHOST_BASE + ((type) << 2))

/* === Global ghost array === */
extern ghost_t ghosts[NUM_GHOSTS];

/* === Scatter target lookup (indexed by ghost type) === */
extern const uint8_t scatter_target_x[NUM_GHOSTS];
extern const uint8_t scatter_target_y[NUM_GHOSTS];

/* === Ghost House constants === */
/* Starting positions (tile coords in 28x31 grid) */
/* Blinky: above door, col 14 row 11 — starts in SCATTER */
/* Pinky:  center slot, col 14 row 14 — starts in HOUSE */
/* Inky:   left slot,   col 12 row 14 — starts in HOUSE */
/* Clyde:  right slot,  col 16 row 14 — starts in HOUSE */

/* Exit point: tile above the door where ghosts transition to normal movement */
#define HOUSE_EXIT_TILE_X  14
#define HOUSE_EXIT_TILE_Y  11

/* Dot limits for Level 1 (personal counters) */
#define DOT_LIMIT_PINKY_L1   0
#define DOT_LIMIT_INKY_L1   30
#define DOT_LIMIT_CLYDE_L1  60

/* Global dot counter thresholds (used after life lost) */
#define GLOBAL_DOT_PINKY    7
#define GLOBAL_DOT_INKY    17
#define GLOBAL_DOT_CLYDE   32

/* Force leave timer: frames without eating before forcing next ghost out. */
#define FORCE_LEAVE_FRAMES  300   /* 4 seconds at 75fps */

/* Frightened mode duration (Level 1): 6 seconds. */
#define FRIGHT_DURATION_L1  450   /* 6 seconds at 75fps */

/* Ghost house speed: half of normal at 75fps */
#define SPD_GHOST_HOUSE     158   /* ~0.62 px/frame at 75fps, half of normal */

/* === Ghost House state (extern, defined in ghost.c) === */
extern uint16_t ghost_house_force_timer;   /* frames since last dot eaten */
extern uint8_t  global_dot_counter_active; /* 1 = use global counter (after life lost) */
extern uint8_t  global_dot_counter;        /* global dot count since life lost */
extern uint16_t fright_timer;          /* frames remaining in frightened mode */
extern uint8_t  num_ghosts_eaten;      /* ghosts eaten with current energizer (0-4) */
extern uint8_t  ghost_eaten_freeze;    /* freeze countdown after eating a ghost */

/* === Function prototypes === */
void ghosts_init(void);
void ghosts_update(void);
void ghosts_render(void);

/* Check collision between ZPac and any ghost.
 * Returns:
 *   0xFF     = no collision
 *   0x00-0x03 = death collision (SCATTER/CHASE ghost)
 *   0x80-0x83 = eat collision (FRIGHTENED ghost, already handled internally)
 */
uint8_t ghosts_check_collision(uint8_t pac_tile_x, uint8_t pac_tile_y);

/* Notify ghost house system that a dot was eaten (called from dots.c) */
void ghost_house_dot_eaten(void);

/* Activate frightened mode on all eligible ghosts (called when energizer eaten) */
void ghosts_enter_frightened(void);

/* Elroy state for Blinky */
extern uint8_t elroy_mode;  /* 0=off, 1=Elroy1, 2=Elroy2 */

#endif /* GHOST_H */
