/* maze_logic.h — Logical maze layout (28x31 grid) */
#ifndef MAZE_LOGIC_H
#define MAZE_LOGIC_H

#include <stdint.h>

extern uint8_t maze_logic[31][28];
extern uint8_t dot_count;

void init_maze_logic(void);

#endif /* MAZE_LOGIC_H */
