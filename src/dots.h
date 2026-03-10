#ifndef DOTS_H
#define DOTS_H

#include <stdint.h>

extern uint32_t score;
extern uint32_t high_score;

void dots_init(void);
void dots_level_reset(void);  /* Reset deferred queue only (keeps score) */
uint8_t dots_check_eat(void);
void dots_update(void);
void score_render(void);

#endif
