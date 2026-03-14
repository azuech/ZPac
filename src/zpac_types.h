/* zpac_types.h — Gameplay data types, cell flags, direction/speed constants */
#ifndef ZPAC_TYPES_H
#define ZPAC_TYPES_H

#include <stdint.h>

/* Cell flags for logic map */
#define CELL_WALKABLE     0x01
#define CELL_DOT          0x02
#define CELL_ENERGIZER    0x04
#define CELL_INTERSECTION 0x08
#define CELL_NO_UP        0x10
#define CELL_TUNNEL       0x20
#define CELL_GHOST_HOUSE  0x40

/* Shortcuts */
#define CELL_W    CELL_WALKABLE
#define CELL_WD   (CELL_WALKABLE | CELL_DOT)
#define CELL_WE   (CELL_WALKABLE | CELL_ENERGIZER)
#define CELL_WT   (CELL_WALKABLE | CELL_TUNNEL)
#define CELL_WG   (CELL_WALKABLE | CELL_GHOST_HOUSE)
#define CELL_WALL 0x00

/* Directions — original arcade convention */
#define DIR_RIGHT  0
#define DIR_DOWN   1
#define DIR_LEFT   2
#define DIR_UP     3
#define DIR_NONE   0xFF

/* Opposite direction */
#define DIR_OPPOSITE(d) (((d) + 2) & 3)

/* Speed patterns — rotating 32-bit accumulator */
/* Each bit=1 out of 32 = the actor moves that frame. Rotated cyclically. */
#define SPEED_ZPAC_NORM_L1   0xD5555555UL  /* 26/32 = ~81% */
#define SPEED_ZPAC_DOT_L1    0x6DB6DB6DUL  /* 23/32 = ~72% */
#define SPEED_ZPAC_FRIGHT_L1 0xEEEEEEEEUL  /* 29/32 = ~91% */
#define SPEED_GHOST_NORM_L1    0x77777777UL  /* 24/32 = 75%  */
#define SPEED_GHOST_FRIGHT_L1  0x55555555UL  /* 16/32 = 50%  */
#define SPEED_GHOST_TUNNEL_L1  0x24922492UL  /* 13/32 = ~40% */

/* ---- Fixed-point 8.8 speeds (256 = 1.0 pixel/frame) ----
 * Base "100% speed" = 307 (1.2 px/frame at 75fps, compensating for 1.5× tile scale).
 * Multiples of 256 = zero jitter. Halves = perfect alternation. */
#define SPD_PAC_NORMAL_L1    346   /* 80% of 433 = 1.35 px/frame at 75fps */
#define SPD_PAC_FRIGHT_L1    390   /* 90% of 433 = 1.52 px/frame */
#define SPD_PAC_DOT_L1       307   /* 71% of 433 = 1.20 px/frame */
#define SPD_GHOST_NORMAL_L1  325   /* 75% of 433 = 1.27 px/frame */
#define SPD_GHOST_FRIGHT_L1  217   /* 50% of 433 = 0.85 px/frame */
#define SPD_GHOST_TUNNEL_L1  173   /* 40% of 433 = 0.68 px/frame */
#define SPD_GHOST_EYES_L1    476   /* ~1.86 px/frame */

/* Get fright flash warning duration for current level (frames at 75fps) */
uint16_t get_fright_flash_frames(void);
#define GHOST_EATEN_FREEZE   75    /* ~1 sec freeze at 75fps */

/* Movement delta per direction: dx[dir], dy[dir] */
/* DIR_RIGHT=0, DIR_DOWN=1, DIR_LEFT=2, DIR_UP=3 */
extern const int8_t dir_dx[4];
extern const int8_t dir_dy[4];

/* === Game Level === */
extern uint8_t game_level;   /* 0-based: 0=Level 1, 1=Level 2, etc. */

/* === Level Speed Table ===
 * 4 speed tiers based on arcade Table A.1.
 * Speeds are fixed-point 8.8 (256 = 1.0 px/frame). */
typedef struct {
    uint16_t pac_normal;
    uint16_t pac_fright;
    uint16_t pac_dot;
    uint16_t ghost_normal;
    uint16_t ghost_fright;
    uint16_t ghost_tunnel;
} speed_set_t;

extern speed_set_t cur_spd;  /* current level speeds, updated on level change */

extern uint8_t lives;
extern uint8_t extra_life_given;

/* Update cur_spd based on game_level */
void update_level_speeds(void);

/* Get frightened duration in frames for current level */
uint16_t get_fright_duration(void);

/* ZPac state (SDCC C89, fields ordered for alignment) */
typedef struct {
    uint16_t speed_acc;    /* speed accumulator fixed-point 8.8 */
    uint8_t  tile_x;       /* tile X in the 28x31 logic grid */
    uint8_t  tile_y;       /* tile Y in the 28x31 logic grid */
    uint8_t  sub_x;        /* X offset within tile (0-11), center=6 */
    uint8_t  sub_y;        /* Y offset within tile (0-11), center=6 */
    uint8_t  dir_current;
    uint8_t  dir_desired;
    uint8_t  anim_tick;
    uint8_t  eat_pause;
} zpac_t;

#endif /* ZPAC_TYPES_H */
