#ifndef CUTSCENE_H
#define CUTSCENE_H

#include <stdint.h>

/* Check if a cutscene should play after completing a level.
 * Call with the NEW game_level (after increment).
 * Returns 1 if a cutscene was played, 0 if not. */
uint8_t cutscene_check_and_play(uint8_t new_level);

#endif
