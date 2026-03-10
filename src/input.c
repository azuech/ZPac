#include <zos_sys.h>
#include <zos_vfs.h>
#include <zos_keyboard.h>
#include "input.h"
#include "zpac_types.h"

void input_init(void) {
    void* arg = (void*)(KB_READ_NON_BLOCK | KB_MODE_RAW);
    ioctl(DEV_STDIN, KB_CMD_SET_MODE, arg);
}

uint8_t input_read(void) {
    uint8_t keys[16];
    uint16_t size = 16;
    uint8_t result = DIR_NONE;

    read(DEV_STDIN, keys, &size);

    if (size == 0)
        return DIR_NONE;

    for (uint8_t i = 0; i < size; i++) {
        if (keys[i] == KB_RELEASED) {
            i++;  /* skip the next byte (the released key) */
        } else {
            switch (keys[i]) {
                case KB_UP_ARROW:    result = DIR_UP;    break;
                case KB_DOWN_ARROW:  result = DIR_DOWN;  break;
                case KB_LEFT_ARROW:  result = DIR_LEFT;  break;
                case KB_RIGHT_ARROW: result = DIR_RIGHT; break;
            }
        }
    }
    return result;
}

uint8_t input_read_menu(void) {
    uint8_t keys[16];
    uint16_t size = 16;
    uint8_t result = DIR_NONE;

    read(DEV_STDIN, keys, &size);

    if (size == 0)
        return DIR_NONE;

    for (uint8_t i = 0; i < size; i++) {
        if (keys[i] == KB_RELEASED) {
            i++;  /* skip the next byte (the released key) */
        } else {
            switch (keys[i]) {
                case KB_UP_ARROW:    result = DIR_UP;      break;
                case KB_DOWN_ARROW:  result = DIR_DOWN;    break;
                case KB_LEFT_ARROW:  result = DIR_LEFT;    break;
                case KB_RIGHT_ARROW: result = DIR_RIGHT;   break;
                case '1':            result = INPUT_COIN;  break;
                case ' ':            result = INPUT_START;  break;
            }
        }
    }
    return result;
}
