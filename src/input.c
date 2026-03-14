#include <zos_sys.h>
#include <zos_vfs.h>
#include <zos_keyboard.h>
#include "input.h"
#include "zpac_types.h"

/* === SNES Controller via PIO Port A === */
__sfr __at 0xD0 pio_data_a;
__sfr __at 0xD2 pio_ctrl_a;

#define PIO_BITCTRL      0xCF
#define PIO_DISABLE_INT  0x03
#define PIN_DATA    0
#define PIN_LATCH   2
#define PIN_CLOCK   3

/* Button bit positions in the 12-bit result (1 = pressed, after inversion).
 * Serial order: B, Y, Select, Start, Up, Down, Left, Right, A, X, L, R.
 * First bit read = MSB (bit 11). */
#define SNES_B       (1u << 11)
#define SNES_Y       (1u << 10)
#define SNES_SELECT  (1u <<  9)
#define SNES_START   (1u <<  8)
#define SNES_UP      (1u <<  7)
#define SNES_DOWN    (1u <<  6)
#define SNES_LEFT    (1u <<  5)
#define SNES_RIGHT   (1u <<  4)
#define SNES_A       (1u <<  3)
#define SNES_X       (1u <<  2)
#define SNES_L       (1u <<  1)
#define SNES_R       (1u <<  0)

static void snes_init(void) {
    /* Set PIO Port A to bit-control mode */
    pio_ctrl_a = PIO_BITCTRL;
    /* Pin direction bitmask: DATA (bit 0) = input (1), rest = output (0) */
    pio_ctrl_a = (uint8_t)(1 << PIN_DATA);
    /* Disable interrupts on this port */
    pio_ctrl_a = PIO_DISABLE_INT;
    /* Default state: CLOCK high, LATCH low */
    pio_data_a = (uint8_t)(1 << PIN_CLOCK);
}

/* Read SNES controller state.
 * Returns 12-bit value, 1 = button pressed (inverted from hardware).
 * If no controller is connected, returns 0 (PIO inputs have pull-ups). */
static uint16_t snes_read(void) {
    uint16_t buttons;
    uint8_t i;

    /* Latch pulse: CLOCK stays high, pulse LATCH high then low */
    pio_data_a = (uint8_t)((1 << PIN_CLOCK) | (1 << PIN_LATCH));
    pio_data_a = (uint8_t)(1 << PIN_CLOCK);

    /* Read first bit (B) — available immediately after latch */
    buttons = (pio_data_a & (1 << PIN_DATA)) ? 0 : 1;

    /* Read remaining 11 bits with clock pulses */
    for (i = 0; i < 11; i++) {
        pio_data_a = 0;                             /* clock low */
        pio_data_a = (uint8_t)(1 << PIN_CLOCK);     /* clock high */
        buttons <<= 1;
        if (!(pio_data_a & (1 << PIN_DATA))) {
            buttons |= 1;
        }
    }

    /* Flush 4 unused bits */
    for (i = 0; i < 4; i++) {
        pio_data_a = 0;
        pio_data_a = (uint8_t)(1 << PIN_CLOCK);
    }

    /* If all 12 buttons read as pressed, no controller is connected
     * (physically impossible on real hardware — emulator or floating pins) */
    if ((buttons & 0x0FFF) == 0x0FFF) return 0;

    return buttons;
}

static uint16_t snes_prev = 0;  /* previous frame SNES state for edge detection */

void input_init(void) {
    void* arg = (void*)(KB_READ_NON_BLOCK | KB_MODE_RAW);
    ioctl(DEV_STDIN, KB_CMD_SET_MODE, arg);
    snes_init();
}

uint8_t input_read(void) {
    uint8_t keys[16];
    uint16_t size = 16;
    uint8_t result = DIR_NONE;
    uint16_t snes;

    /* 1. Read keyboard buffer (always drain to prevent stale input) */
    read(DEV_STDIN, keys, &size);

    if (size > 0) {
        uint8_t i;
        for (i = 0; i < size; i++) {
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
    }

    /* 2. Read SNES controller — d-pad overrides keyboard if pressed */
    snes = snes_read();
    snes_prev = snes;  /* keep edge state in sync */
    if (snes & SNES_UP)    return DIR_UP;
    if (snes & SNES_DOWN)  return DIR_DOWN;
    if (snes & SNES_LEFT)  return DIR_LEFT;
    if (snes & SNES_RIGHT) return DIR_RIGHT;

    return result;
}

uint8_t input_read_menu(void) {
    uint8_t keys[16];
    uint16_t size = 16;
    uint8_t result = DIR_NONE;
    uint16_t snes;

    /* 1. Read keyboard buffer */
    read(DEV_STDIN, keys, &size);

    if (size > 0) {
        uint8_t i;
        for (i = 0; i < size; i++) {
            if (keys[i] == KB_RELEASED) {
                i++;
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
    }

    /* 2. Read SNES controller — edge detection for Select/Start */
    snes = snes_read();
    {
        uint16_t pressed = snes & ~snes_prev;  /* newly pressed this frame */
        snes_prev = snes;
        /* D-pad: use held state (continuous reading, like keyboard autorepeat) */
        if (snes & SNES_UP)       return DIR_UP;
        if (snes & SNES_DOWN)     return DIR_DOWN;
        if (snes & SNES_LEFT)     return DIR_LEFT;
        if (snes & SNES_RIGHT)    return DIR_RIGHT;
        /* Buttons: use edge detection (fire once per press) */
        if (pressed & SNES_SELECT) return INPUT_COIN;
        if (pressed & SNES_START)  return INPUT_START;
    }

    return result;
}
