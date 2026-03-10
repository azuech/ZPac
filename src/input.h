#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

/* Initialize keyboard in RAW + NON_BLOCK mode */
void input_init(void);

/* Read input and return DIR_RIGHT/DOWN/LEFT/UP or DIR_NONE */
uint8_t input_read(void);

/* Special input constants for menu */
#define INPUT_COIN   0xF0
#define INPUT_START  0xF1

/* Read input for menu screens: recognizes arrows + '1' (coin) + space (start) */
uint8_t input_read_menu(void);

#endif
