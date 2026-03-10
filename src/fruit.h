#ifndef FRUIT_H
#define FRUIT_H

#include <stdint.h>

/* Fruit type indices (0-7) */
#define FRUIT_CHERRY      0
#define FRUIT_STRAWBERRY  1
#define FRUIT_PEACH       2
#define FRUIT_APPLE       3
#define FRUIT_GRAPES      4
#define FRUIT_GALAXIAN    5
#define FRUIT_BELL        6
#define FRUIT_KEY         7
#define NUM_FRUIT_TYPES   8

/* Hardware sprite base for fruit (2x2 composite, sprites 20-23) */
#define HW_SPR_FRUIT_BASE  20

/* Initialize fruit state (call at game start and level start) */
void fruit_init(void);

/* Check if fruit should spawn (call after each dot eaten).
 * dots_eaten = 244 - dot_count */
void fruit_check_spawn(uint8_t dots_eaten);

/* Update fruit timer, check collision with ZPac (call each frame) */
void fruit_update(void);

/* Render fruit sprite (call each frame after fruit_update) */
void fruit_render(void);

/* Hide fruit sprites off-screen (call during death/level complete) */
void fruit_hide(void);

/* Render fruit level icons in HUD bottom-right (call after level
 * changes and at game start) */
void fruit_hud_render(void);

/* Hide fruit HUD icons (call during GAME OVER) */
void fruit_hud_hide(void);

#endif /* FRUIT_H */
