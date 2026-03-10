#ifndef LEVEL256_H
#define LEVEL256_H

#include <stdint.h>

/* Apply level 256 corruption to maze logic and tilemap.
 * Call AFTER init_maze_logic() and redraw_maze() when game_level==255.
 * Also call on death respawn to re-place the 8 dots (arcade behavior). */
void level256_apply(void);

/* Flag: 1 if current level is the corrupted level 256 */
extern uint8_t is_level_256;

#endif
