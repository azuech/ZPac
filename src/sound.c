/* sound.c — ZPac audio engine
 *
 * Architecture: all audio register writes happen inside sound_update()
 * or bracketed by peripheral bank save/restore. This prevents the
 * bank conflict between sound (bank 3) and graphics (bank 0) that
 * causes noise artifacts.
 */

#include <zvb_gfx.h>
#include <zvb_sound.h>
#include "sound.h"
#include "zpac_types.h"

/* === Sound states === */
#define SND_SILENT   0
#define SND_SIREN    1
#define SND_WAKA     2    /* waka sound active on voice 1 */
#define SND_FRIGHT   3

/* === Siren parameters (tuned) ===
 * Step  87 per frame (0x0057), half-cycle = 10 frames (0.17s)
 * Full cycle = 20 frames (0.33s) — fast "uauauaua" sound. */
#define SIREN_FREQ_LOW    0x022E
#define SIREN_FREQ_HIGH   0x0575
#define SIREN_STEP        87

/* === Waka-waka parameters ===
 * Two alternating sounds on VOICE1:
 * eatdot1: starts high (0x02ED, ~496Hz), descends 0x68/frame for 5 frames
 * eatdot2: starts low  (0x00F7, ~164Hz), ascends  0x68/frame for 5 frames
 * Waveform: square 50% duty (square is closer on Zeal PSG
 * for a "crunchy" eating sound). */
#define WAKA_FREQ_HIGH    0x02ED
#define WAKA_FREQ_LOW     0x00F7
#define WAKA_STEP         0x0068
#define WAKA_DURATION     5

/* === Fright sound parameters (tuned for Zeal PSG) ===
 * Rapid ascending tone on VOICE0, replaces siren during frightened mode.
 * Resets to base frequency every 8 frames.
 * Base freq: ~140 Hz = 0x00D0 (Zeal PSG)
 * Step: 0x00D0 per frame
 * After 7 frames: 0x0680 (~1120 Hz), then reset.
 * Waveform: sawtooth. */
#define FRIGHT_FREQ_BASE  0x00D0
#define FRIGHT_STEP       0x00D0
#define FRIGHT_CYCLE      8

/* === Ghost eaten parameters ===
 * Rising sweep on VOICE2, one-shot.
 * Start: 0x0060 (~64 Hz), step: 0x0060/frame
 * End: 0x0C00 (~2050 Hz) after 31 frames.
 * Duration: 32 frames (~0.53s at 60fps). */
#define GHOST_EAT_FREQ_START  0x0030
#define GHOST_EAT_STEP        0x0030
#define GHOST_EAT_DURATION    32

/* === State variables === */
static uint8_t  snd_state;
static uint16_t siren_freq;
static int8_t   siren_dir;      /* +1 = rising, -1 = falling */
static uint8_t  waka_active;    /* 1 = waka sound playing on voice 1 */
static uint8_t  waka_timer;     /* frames remaining for current waka */
static uint8_t  waka_toggle;    /* 0 = eatdot1 (descending), 1 = eatdot2 (ascending) */
static uint16_t waka_freq;      /* current frequency */
static uint8_t  fright_tick;       /* tick counter within 8-frame cycle */
static uint16_t fright_freq;       /* current fright frequency */
static uint16_t fright_countdown;  /* frames remaining in frightened mode */
static uint8_t  ghost_eat_active;   /* 1 = ghost eat sound playing on voice 2 */
static uint8_t  ghost_eat_timer;    /* frames remaining */
static uint16_t ghost_eat_freq;     /* current frequency */
static uint8_t  fruit_snd_active;   /* 1 = fruit eat sound playing on voice 2 */
static uint8_t  fruit_snd_timer;    /* tick counter */
static uint16_t fruit_snd_freq;     /* current frequency */
#define FRUIT_SND_FREQ_START  0x02FE
#define FRUIT_SND_FREQ_STEP   0x004A
#define FRUIT_SND_DURATION    24

/* === Prelude data ===
 * 245 ticks at 60fps = ~4.08 seconds.
 * Voice 0 = bass (sawtooth, volume fades per note)
 * Voice 1 = melody (square 50%, full volume or silence)
 * Encoding: 1 byte per tick, high nibble = v0 freq index, low = v1 freq index.
 * Index 0 = silence for both voices. */

/* Voice 0 frequency LUT (bass line, waveform 2 = 2 cycles) */
static const uint16_t prelude_v0_lut[8] = {
    0x0000,  /* idx 0: silence */
    0x00C8,  /* idx 1: 135 Hz */
    0x00D3,  /* idx 2: 142 Hz */
    0x012C,  /* idx 3: 202 Hz */
    0x013E,  /* idx 4: 214 Hz */
    0x014F,  /* idx 5: 226 Hz */
    0x0179,  /* idx 6: 253 Hz */
    0x0191,  /* idx 7: 270 Hz */
};

/* Voice 1 frequency LUT (melody) */
static const uint16_t prelude_v1_lut[13] = {
    0x0000,  /* idx 0: silence */
    0x01F9,  /* idx 1: 340 Hz */
    0x0321,  /* idx 2: 539 Hz */
    0x034D,  /* idx 3: 568 Hz */
    0x03B5,  /* idx 4: 639 Hz */
    0x03F2,  /* idx 5: 680 Hz */
    0x042F,  /* idx 6: 721 Hz */
    0x046C,  /* idx 7: 762 Hz */
    0x04B2,  /* idx 8: 809 Hz */
    0x04F8,  /* idx 9: 855 Hz */
    0x053D,  /* idx 10: 902 Hz */
    0x0643,  /* idx 11: 1078 Hz */
    0x06A2,  /* idx 12: 1143 Hz */
};

#define PRELUDE_NUM_TICKS  245

/* Per-tick note indices: high nibble = v0 idx, low nibble = v1 idx */
static const uint8_t prelude_data[245] = {
    0x12,0x12,0x12,0x12,0x10,0x10,0x10,0x10,0x1B,0x1B,0x1B,0x1B,0x10,0x10,0x10,0x08,
    0x08,0x08,0x08,0x00,0x00,0x00,0x00,0x35,0x35,0x35,0x35,0x30,0x30,0x30,0x30,0x1B,
    0x1B,0x1B,0x1B,0x18,0x18,0x18,0x18,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x05,0x05,
    0x05,0x05,0x05,0x05,0x05,0x05,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x23,0x23,
    0x23,0x23,0x20,0x20,0x20,0x20,0x2C,0x2C,0x2C,0x2C,0x20,0x20,0x20,0x09,0x09,0x09,
    0x09,0x00,0x00,0x00,0x00,0x46,0x46,0x46,0x46,0x40,0x40,0x40,0x40,0x2C,0x2C,0x2C,
    0x2C,0x29,0x29,0x29,0x29,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x06,0x06,0x06,0x06,
    0x06,0x06,0x06,0x06,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x12,0x12,0x12,0x12,
    0x10,0x10,0x10,0x10,0x1B,0x1B,0x1B,0x1B,0x10,0x10,0x10,0x08,0x08,0x08,0x08,0x00,
    0x00,0x00,0x00,0x35,0x35,0x35,0x35,0x30,0x30,0x30,0x30,0x1B,0x1B,0x1B,0x1B,0x18,
    0x18,0x18,0x18,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x05,0x05,0x05,0x05,0x05,0x05,
    0x05,0x05,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x34,0x34,0x34,0x34,0x35,0x35,
    0x35,0x35,0x36,0x36,0x36,0x36,0x30,0x30,0x30,0x56,0x56,0x56,0x56,0x57,0x57,0x57,
    0x57,0x58,0x58,0x58,0x58,0x50,0x50,0x50,0x68,0x68,0x68,0x68,0x69,0x69,0x69,0x69,
    0x6A,0x6A,0x6A,0x6A,0x60,0x60,0x60,0x7B,0x7B,0x7B,0x7B,0x7B,0x7B,0x7B,0x7B,0x70,
    0x70,0x70,0x70,0x70,0x70,
};

/* Voice 0 volume fade mapping: vol 14->0 maps to Zeal 4 levels.
 * Index = tick within note (0-14), value = Zeal volume register value. */
static const uint8_t prelude_v0_vol[16] = {
    VOL_75, VOL_75, VOL_75, VOL_75,     /* vol 14-11 */
    VOL_50, VOL_50, VOL_50, VOL_50,     /* vol 10-7 */
    VOL_25, VOL_25, VOL_25, VOL_25,     /* vol 6-3 */
    VOL_0,  VOL_0,  VOL_0,  VOL_0       /* vol 2-0 + padding */
};

/* === Peripheral bank save/restore ===
 * zvb_config_dev_idx is an extern uint8_t in the SDK tracking current bank.
 * zvb_map_peripheral() switches the banked I/O to the specified device. */
static uint8_t saved_bank;

static void snd_bank_save(void) {
    saved_bank = zvb_config_dev_idx;
}

static void snd_bank_restore(void) {
    zvb_map_peripheral(saved_bank);
}

/* === Public API === */

void sound_init(void) {
    snd_state = SND_SILENT;
    siren_freq = SIREN_FREQ_LOW;
    siren_dir = 1;
    waka_active = 0;
    waka_timer = 0;
    waka_toggle = 0;
    waka_freq = 0;
    fright_tick = 0;
    fright_freq = 0;
    fright_countdown = 0;
    ghost_eat_active = 0;
    ghost_eat_timer = 0;
    ghost_eat_freq = 0;
    fruit_snd_active = 0;
    fruit_snd_timer = 0;
    fruit_snd_freq = 0;
    /* Full PSG reset — all voices held, volumes zeroed */
    zvb_sound_initialize(1);
    zvb_sound_set_voices_vol(VOICE0 | VOICE1 | VOICE2 | VOICE3, VOL_0);
    zvb_sound_set_volumes(VOL_0, VOL_0);

    /* Restore bank to whatever it was before (likely gfx) */
    snd_bank_restore();
}

void sound_start_siren(void) {
    snd_bank_save();

    snd_state = SND_SIREN;
    siren_freq = SIREN_FREQ_LOW;
    siren_dir = 1;

    /* Configure voice 0: square wave 50% duty, starting frequency */
    zvb_sound_set_voices(VOICE0, siren_freq, WAV_TRIANGLE);
    zvb_sound_set_channels(VOICE0, VOICE0);
    zvb_sound_set_voices_vol(VOICE0, VOL_25);
    zvb_sound_set_volume(VOL_100);
    zvb_sound_set_hold(VOICE0, 0);

    snd_bank_restore();
}

void sound_stop_all(void) {
    snd_bank_save();

    snd_state = SND_SILENT;
    waka_active = 0;
    fright_countdown = 0;
    ghost_eat_active = 0;
    fruit_snd_active = 0;

    /* Disable all voice volumes */
    zvb_sound_set_voices_vol(VOICE0 | VOICE1 | VOICE2 | VOICE3, VOL_0);
    /* Disable master volume on both channels */
    zvb_sound_set_volumes(VOL_0, VOL_0);

    snd_bank_restore();
}

void sound_waka(void) {
    snd_bank_save();

    waka_active = 1;
    waka_timer = WAKA_DURATION;

    /* Alternate between descending (eatdot1) and ascending (eatdot2) */
    if (waka_toggle) {
        waka_freq = WAKA_FREQ_LOW;
    } else {
        waka_freq = WAKA_FREQ_HIGH;
    }
    waka_toggle ^= 1;

    /* Configure voice 1 and set frequency + volume via direct registers.
     * We call set_voices once to set waveform, then override freq/vol directly
     * to avoid repeated hold/unhold on subsequent calls. */
    zvb_map_peripheral(ZVB_PERI_SOUND_IDX);
    zvb_peri_sound_select = VOICE1;
    zvb_peri_sound_wave = DUTY_CYCLE_50_0 | WAV_SQUARE;
    zvb_peri_sound_freq_low = waka_freq & 0xFF;
    zvb_peri_sound_freq_high = (waka_freq >> 8) & 0xFF;
    zvb_peri_sound_volume = VOL_75;
    /* Ensure voice 1 is on both channels */
    zvb_peri_sound_left_channel = VOICE0 | VOICE1;
    zvb_peri_sound_right_channel = VOICE0 | VOICE1;

    snd_bank_restore();
}

void sound_ghost_eaten(void) {
    snd_bank_save();

    ghost_eat_active = 1;
    ghost_eat_timer = GHOST_EAT_DURATION;
    ghost_eat_freq = GHOST_EAT_FREQ_START;

    /* Configure voice 2: sawtooth, starting frequency, full volume */
    zvb_map_peripheral(ZVB_PERI_SOUND_IDX);
    zvb_peri_sound_select = VOICE2;
    zvb_peri_sound_wave = WAV_SAWTOOTH;
    zvb_peri_sound_freq_low = ghost_eat_freq & 0xFF;
    zvb_peri_sound_freq_high = (ghost_eat_freq >> 8) & 0xFF;
    zvb_peri_sound_volume = VOL_100;
    /* Ensure voice 2 is on both channels */
    zvb_peri_sound_left_channel = VOICE0 | VOICE1 | VOICE2;
    zvb_peri_sound_right_channel = VOICE0 | VOICE1 | VOICE2;

    snd_bank_restore();
}

void sound_start_fright(void) {
    snd_bank_save();

    snd_state = SND_FRIGHT;
    fright_tick = 0;
    fright_freq = FRIGHT_FREQ_BASE;
    /* Get fright duration from the level speed system */
    fright_countdown = get_fright_duration();

    /* Configure voice 0: sawtooth wave at base frequency */
    zvb_map_peripheral(ZVB_PERI_SOUND_IDX);
    zvb_peri_sound_select = VOICE0;
    zvb_peri_sound_wave = WAV_SAWTOOTH;
    zvb_peri_sound_freq_low = fright_freq & 0xFF;
    zvb_peri_sound_freq_high = (fright_freq >> 8) & 0xFF;
    zvb_peri_sound_volume = VOL_50;

    snd_bank_restore();
}

void sound_play_prelude(void) {
    extern gfx_context vctx;
    uint8_t prev_v0_idx = 0xFF;
    uint8_t prev_v1_idx = 0xFF;
    uint8_t v0_note_tick = 0;

    snd_bank_save();

    /* Initialize PSG for prelude */
    zvb_sound_initialize(1);
    zvb_sound_set_channels(VOICE0 | VOICE1, VOICE0 | VOICE1);
    zvb_sound_set_volume(VOL_100);

    /* Setup voice 0: sawtooth (bass) */
    zvb_sound_set_voices(VOICE0, 0, WAV_SAWTOOTH);
    zvb_sound_set_voices_vol(VOICE0, VOL_0);
    zvb_sound_set_hold(VOICE0, 0);

    /* Setup voice 1: square 50% (melody) */
    zvb_sound_set_voices(VOICE1, 0, DUTY_CYCLE_50_0 | WAV_SQUARE);
    zvb_sound_set_voices_vol(VOICE1, VOL_0);
    zvb_sound_set_hold(VOICE1, 0);

    snd_bank_restore();

    /* Play with fractional accumulator: 129/100 ticks per frame
     * to match original timing (4.36s vs 5.63s, ratio ~0.775) */
    {
        uint16_t tick = 0;
        uint8_t acc = 0;

        while (tick < PRELUDE_NUM_TICKS) {
            /* Wait one display frame */
            gfx_wait_vblank(&vctx);
            gfx_wait_end_vblank(&vctx);

            /* Accumulator: process 1 or 2 ticks per frame */
            acc += 119;

            while (acc >= 100 && tick < PRELUDE_NUM_TICKS) {
                uint8_t data_byte;
                uint8_t v0_idx;
                uint8_t v1_idx;
                uint16_t v0_freq;
                uint16_t v1_freq;
                uint8_t v0_vol;
                uint8_t v1_vol;

                acc -= 100;

                data_byte = prelude_data[tick];
                v0_idx = (data_byte >> 4) & 0x0F;
                v1_idx = data_byte & 0x0F;
                v0_freq = prelude_v0_lut[v0_idx];
                v1_freq = (v1_idx < 13) ? prelude_v1_lut[v1_idx] : 0;

                /* Voice 0 volume fade */
                if (v0_idx != prev_v0_idx) {
                    v0_note_tick = 0;
                    prev_v0_idx = v0_idx;
                }
                if (v0_idx == 0) {
                    v0_vol = VOL_0;
                } else {
                    v0_vol = (v0_note_tick < 16) ? prelude_v0_vol[v0_note_tick] : VOL_0;
                    v0_note_tick++;
                }

                /* Voice 1: full volume when playing, off when silent */
                v1_vol = (v1_idx != 0) ? VOL_100 : VOL_0;

                /* Write to PSG registers directly */
                zvb_map_peripheral(ZVB_PERI_SOUND_IDX);

                zvb_peri_sound_select = VOICE0;
                zvb_peri_sound_freq_low = v0_freq & 0xFF;
                zvb_peri_sound_freq_high = (v0_freq >> 8) & 0xFF;
                zvb_peri_sound_volume = v0_vol;

                zvb_peri_sound_select = VOICE1;
                zvb_peri_sound_freq_low = v1_freq & 0xFF;
                zvb_peri_sound_freq_high = (v1_freq >> 8) & 0xFF;
                zvb_peri_sound_volume = v1_vol;

                snd_bank_restore();

                tick++;
            }
        }
    }

    /* Silence all after prelude */
    snd_bank_save();
    zvb_sound_set_voices_vol(VOICE0 | VOICE1 | VOICE2 | VOICE3, VOL_0);
    zvb_sound_set_volumes(VOL_0, VOL_0);
    snd_bank_restore();
}

/* === Death jingle ===
 * 90 ticks of register dump, converted from WSG to Zeal PSG.
 * Tick 0-66: triangle wave, oscillating descending phrases.
 * Tick 67-77: square wave, ascending sweep ("tail").
 * Tick 78: silence.
 * Tick 79-89: repeat ascending sweep. */
#define DEATH_SND_TICKS  90

static const uint16_t death_snd_freq[DEATH_SND_TICKS] = {
    0x0437, 0x0415, 0x03F2, 0x03CF, 0x03AC, 0x03CF,
    0x03F2, 0x0415, 0x0437, 0x045A, 0x047D, 0x03F2,
    0x03CF, 0x03AC, 0x0389, 0x0366, 0x0344, 0x0366,
    0x0389, 0x03AC, 0x03CF, 0x03F2, 0x0415, 0x03AC,
    0x0389, 0x0366, 0x0344, 0x0321, 0x02FE, 0x0321,
    0x0344, 0x0366, 0x0389, 0x03AC, 0x03CF, 0x0366,
    0x0344, 0x0321, 0x02FE, 0x02DB, 0x02B8, 0x02DB,
    0x02FE, 0x0321, 0x0344, 0x0366, 0x0389, 0x0321,
    0x02FE, 0x02DB, 0x02B8, 0x0295, 0x0273, 0x0295,
    0x02B8, 0x02DB, 0x02FE, 0x0321, 0x0344, 0x02DB,
    0x02B8, 0x0295, 0x0273, 0x0250, 0x022D, 0x0250,
    0x0273, 0x0116, 0x022D, 0x0344, 0x045A, 0x0571,
    0x0688, 0x079E, 0x08B5, 0x09CC, 0x0AE2, 0x0BF9,
    0x0000, 0x0116, 0x022D, 0x0344, 0x045A, 0x0571,
    0x0688, 0x079E, 0x08B5, 0x09CC, 0x0AE2, 0x0BF9
};

/* Volume: 13-15=VOL_100, 10-12=VOL_75, 5-8=VOL_50, 0=VOL_0 */
static const uint8_t death_snd_vol[DEATH_SND_TICKS] = {
    VOL_100, VOL_100, VOL_100, VOL_100, VOL_100, VOL_100,
    VOL_100, VOL_100, VOL_100, VOL_100, VOL_100, VOL_100,
    VOL_100, VOL_100, VOL_100, VOL_100, VOL_100, VOL_100,
    VOL_100, VOL_100, VOL_100, VOL_100, VOL_100, VOL_100,
    VOL_100, VOL_100, VOL_100, VOL_100, VOL_100, VOL_100,
    VOL_100, VOL_100, VOL_100, VOL_100, VOL_100, VOL_75,
    VOL_75,  VOL_75,  VOL_75,  VOL_75,  VOL_75,  VOL_75,
    VOL_75,  VOL_75,  VOL_75,  VOL_75,  VOL_75,  VOL_75,
    VOL_75,  VOL_75,  VOL_75,  VOL_75,  VOL_75,  VOL_75,
    VOL_75,  VOL_75,  VOL_75,  VOL_75,  VOL_75,  VOL_75,
    VOL_75,  VOL_75,  VOL_75,  VOL_75,  VOL_75,  VOL_75,
    VOL_75,  VOL_50,  VOL_50,  VOL_50,  VOL_50,  VOL_50,
    VOL_50,  VOL_50,  VOL_50,  VOL_50,  VOL_50,  VOL_50,
    VOL_0,   VOL_50,  VOL_50,  VOL_50,  VOL_50,  VOL_50,
    VOL_50,  VOL_50,  VOL_50,  VOL_50,  VOL_50,  VOL_0
};

/* Waveform: 1=triangle (tick 0-66), 0=square (tick 67-89) */
static const uint8_t death_snd_wave[DEATH_SND_TICKS] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0
};

/* Intermission music — RLE compressed.
 * Format: {freq_hi, freq_lo, duration} triplets.
 * Total ticks per repetition = 329. Played twice. */
#define INTERMISSION_NUM_TICKS  329
#define INTM_MELODY_RUNS  29
#define INTM_BASS_RUNS    30

static const uint8_t intm_melody_rle[87] = {
    0x01,0x03, 31,
    0x00,0xDA,  6,
    0x00,0xC2,  5,
    0x01,0x03, 16,
    0x01,0x46, 21,
    0x00,0xC2,  5,
    0x01,0x03, 32,
    0x00,0xDA,  5,
    0x00,0xC2,  6,
    0x01,0x03, 15,
    0x00,0xDA, 22,
    0x00,0xC2,  5,
    0x01,0x03, 32,
    0x00,0xDA,  5,
    0x00,0xC2,  5,
    0x01,0x03, 16,
    0x01,0x34,  5,
    0x00,0xA3,  6,
    0x01,0x59,  5,
    0x00,0xB7,  5,
    0x01,0x6E, 16,
    0x01,0x59,  5,
    0x00,0xC2,  6,
    0x01,0x34,  5,
    0x00,0xAD,  5,
    0x01,0x03, 11,
    0x01,0x34, 16,
    0x01,0x03, 16,
    0x00,0x00,  1
};

static const uint8_t intm_bass_rle[90] = {
    0x00,0x81,  5,
    0x00,0x00, 10,
    0x00,0x81, 11,
    0x00,0x00, 11,
    0x00,0x81, 10,
    0x00,0x00, 11,
    0x00,0x81,  5,
    0x00,0x00, 21,
    0x00,0x81,  5,
    0x00,0x00, 11,
    0x00,0x81, 11,
    0x00,0x00, 10,
    0x00,0x81, 11,
    0x00,0x00, 10,
    0x00,0x81,  6,
    0x00,0x00, 21,
    0x00,0x81,  5,
    0x00,0x00, 11,
    0x00,0x81, 10,
    0x00,0x00, 11,
    0x00,0x81, 11,
    0x00,0x00, 10,
    0x00,0x81,  5,
    0x00,0x00, 53,
    0x00,0x9A,  6,
    0x00,0x81,  5,
    0x00,0x73, 11,
    0x00,0x7A, 10,
    0x00,0x81, 11,
    0x00,0x00,  1
};

static uint16_t intermission_tick;
static uint8_t  intermission_loop;
static uint8_t  intm_mel_run;   /* current run index in melody RLE */
static uint8_t  intm_mel_rem;   /* remaining ticks in current melody run */
static uint16_t intm_mel_freq;  /* current melody frequency */
static uint8_t  intm_bas_run;   /* current run index in bass RLE */
static uint8_t  intm_bas_rem;   /* remaining ticks in current bass run */
static uint16_t intm_bas_freq;  /* current bass frequency */

static uint8_t death_snd_tick;  /* current tick in the death sequence */

void sound_death_start(void) {
    snd_bank_save();

    snd_state = SND_SILENT;
    waka_active = 0;
    ghost_eat_active = 0;
    death_snd_tick = 0;

    /* Silence everything first */
    zvb_sound_set_voices_vol(VOICE0 | VOICE1 | VOICE2 | VOICE3, VOL_0);

    /* Configure VOICE0: triangle wave, first frequency */
    zvb_sound_set_voices(VOICE0, death_snd_freq[0], WAV_TRIANGLE);
    zvb_sound_set_channels(VOICE0, VOICE0);
    zvb_sound_set_voices_vol(VOICE0, VOL_100);
    zvb_sound_set_volume(VOL_100);
    zvb_sound_set_hold(VOICE0, 0);

    snd_bank_restore();
}

void sound_death_update(uint8_t unused) {
    uint16_t freq;
    uint8_t vol;
    (void)unused;  /* parameter kept for API compat, we use internal tick */

    if (death_snd_tick >= DEATH_SND_TICKS) {
        /* Auto-silence when dump ends */
        snd_bank_save();
        zvb_map_peripheral(ZVB_PERI_SOUND_IDX);
        zvb_peri_sound_select = VOICE0;
        zvb_peri_sound_volume = VOL_0;
        snd_bank_restore();
        return;
    }

    freq = death_snd_freq[death_snd_tick];
    vol = death_snd_vol[death_snd_tick];

    snd_bank_save();
    zvb_map_peripheral(ZVB_PERI_SOUND_IDX);
    zvb_peri_sound_select = VOICE0;
    zvb_peri_sound_freq_low = freq & 0xFF;
    zvb_peri_sound_freq_high = (freq >> 8) & 0xFF;
    zvb_peri_sound_volume = vol;
    /* Switch waveform when changing from triangle to square */
    if (death_snd_tick == 67) {
        zvb_peri_sound_wave = DUTY_CYCLE_50_0 | WAV_SQUARE;
    }
    snd_bank_restore();

    death_snd_tick++;
}

void sound_fruit_eaten(void) {
    snd_bank_save();

    fruit_snd_active = 1;
    fruit_snd_timer = 0;
    fruit_snd_freq = FRUIT_SND_FREQ_START;

    /* Configure VOICE2: 75% duty square wave */
    zvb_map_peripheral(ZVB_PERI_SOUND_IDX);
    zvb_peri_sound_select = VOICE2;
    zvb_peri_sound_wave = DUTY_CYCLE_75_0 | WAV_SQUARE;
    zvb_peri_sound_freq_low = fruit_snd_freq & 0xFF;
    zvb_peri_sound_freq_high = (fruit_snd_freq >> 8) & 0xFF;
    zvb_peri_sound_volume = VOL_100;
    /* Ensure voice 2 on both channels */
    zvb_peri_sound_left_channel = VOICE0 | VOICE1 | VOICE2;
    zvb_peri_sound_right_channel = VOICE0 | VOICE1 | VOICE2;
    /* Restore master volume (needed if called from title after sound_stop_all) */
    zvb_peri_sound_master_vol = (VOL_100 << 4) | VOL_100;
    /* Unhold voice 2 */
    zvb_peri_sound_hold = 0;

    snd_bank_restore();
}

void sound_death_stop(void) {
    snd_bank_save();
    zvb_sound_set_voices_vol(VOICE0 | VOICE1 | VOICE2 | VOICE3, VOL_0);
    zvb_sound_set_volumes(VOL_0, VOL_0);
    snd_bank_restore();
}

/* Decode one RLE run: read freq (2 bytes) and duration (1 byte) */
static void intm_decode_run(const uint8_t *rle, uint8_t run_idx,
                             uint16_t *freq, uint8_t *dur) {
    uint8_t off = run_idx * 3;
    *freq = ((uint16_t)rle[off] << 8) | rle[off + 1];
    *dur = rle[off + 2];
}

void sound_intermission_start(void) {
    snd_bank_save();

    snd_state = SND_SILENT;
    waka_active = 0;
    ghost_eat_active = 0;
    fruit_snd_active = 0;
    intermission_tick = 0;
    intermission_loop = 0;

    /* Initialize RLE decoders */
    intm_mel_run = 0;
    intm_decode_run(intm_melody_rle, 0, &intm_mel_freq, &intm_mel_rem);
    intm_bas_run = 0;
    intm_decode_run(intm_bass_rle, 0, &intm_bas_freq, &intm_bas_rem);

    /* Initialize PSG: silence everything first */
    zvb_sound_initialize(1);
    zvb_sound_set_channels(VOICE0 | VOICE1, VOICE0 | VOICE1);
    zvb_sound_set_volume(VOL_100);

    /* Voice 0: melody — square wave 50% duty */
    zvb_sound_set_voices(VOICE0, 0, DUTY_CYCLE_50_0 | WAV_SQUARE);
    zvb_sound_set_voices_vol(VOICE0, VOL_100);
    zvb_sound_set_hold(VOICE0, 0);

    /* Voice 1: bass — sawtooth */
    zvb_sound_set_voices(VOICE1, 0, WAV_SAWTOOTH);
    zvb_sound_set_voices_vol(VOICE1, VOL_75);
    zvb_sound_set_hold(VOICE1, 0);

    snd_bank_restore();
}

uint8_t sound_intermission_update(void) {
    uint16_t mel_freq;
    uint16_t bas_freq;
    uint8_t mel_vol;
    uint8_t bas_vol;

    if (intermission_tick >= INTERMISSION_NUM_TICKS) {
        intermission_loop++;
        if (intermission_loop >= 2) {
            /* Played twice — silence and return 1 (finished) */
            snd_bank_save();
            zvb_sound_set_voices_vol(VOICE0 | VOICE1 | VOICE2 | VOICE3, VOL_0);
            zvb_sound_set_volumes(VOL_0, VOL_0);
            snd_bank_restore();
            return 1;
        }
        /* Loop: restart from beginning */
        intermission_tick = 0;
        /* Reset RLE decoders for new loop */
        intm_mel_run = 0;
        intm_decode_run(intm_melody_rle, 0, &intm_mel_freq, &intm_mel_rem);
        intm_bas_run = 0;
        intm_decode_run(intm_bass_rle, 0, &intm_bas_freq, &intm_bas_rem);
    }

    mel_freq = intm_mel_freq;
    bas_freq = intm_bas_freq;

    mel_vol = (mel_freq > 0) ? VOL_100 : VOL_0;
    bas_vol = (bas_freq > 0) ? VOL_75 : VOL_0;

    snd_bank_save();
    zvb_map_peripheral(ZVB_PERI_SOUND_IDX);

    /* Voice 0: melody */
    zvb_peri_sound_select = VOICE0;
    zvb_peri_sound_freq_low = mel_freq & 0xFF;
    zvb_peri_sound_freq_high = (mel_freq >> 8) & 0xFF;
    zvb_peri_sound_volume = mel_vol;

    /* Voice 1: bass */
    zvb_peri_sound_select = VOICE1;
    zvb_peri_sound_freq_low = bas_freq & 0xFF;
    zvb_peri_sound_freq_high = (bas_freq >> 8) & 0xFF;
    zvb_peri_sound_volume = bas_vol;

    snd_bank_restore();

    /* Advance RLE decoders */
    intm_mel_rem--;
    if (intm_mel_rem == 0 && intm_mel_run < INTM_MELODY_RUNS - 1) {
        intm_mel_run++;
        intm_decode_run(intm_melody_rle, intm_mel_run,
                         &intm_mel_freq, &intm_mel_rem);
    }
    intm_bas_rem--;
    if (intm_bas_rem == 0 && intm_bas_run < INTM_BASS_RUNS - 1) {
        intm_bas_run++;
        intm_decode_run(intm_bass_rle, intm_bas_run,
                         &intm_bas_freq, &intm_bas_rem);
    }

    intermission_tick++;
    return 0;
}

void sound_update(void) {
    /* Early exit if nothing to do — avoids unnecessary bank switching */
    if (snd_state == SND_SILENT && !waka_active && !ghost_eat_active && !fruit_snd_active) {
        return;
    }

    snd_bank_save();

    if (snd_state == SND_SIREN) {
        /* Oscillate frequency up and down */
        if (siren_dir > 0) {
            siren_freq += SIREN_STEP;
            if (siren_freq >= SIREN_FREQ_HIGH) {
                siren_freq = SIREN_FREQ_HIGH;
                siren_dir = -1;
            }
        } else {
            if (siren_freq <= SIREN_FREQ_LOW + SIREN_STEP) {
                siren_freq = SIREN_FREQ_LOW;
                siren_dir = 1;
            } else {
                siren_freq -= SIREN_STEP;
            }
        }
        /* Update frequency directly via registers — no hold/unhold cycle.
         * Hardware latches freq changes at end of waveform period. */
        zvb_map_peripheral(ZVB_PERI_SOUND_IDX);
        zvb_peri_sound_select = VOICE0;
        zvb_peri_sound_freq_low = siren_freq & 0xFF;
        zvb_peri_sound_freq_high = (siren_freq >> 8) & 0xFF;
    }

    if (snd_state == SND_FRIGHT) {
        /* Count down fright duration */
        if (fright_countdown > 0) {
            fright_countdown--;
        }
        if (fright_countdown == 0) {
            /* Fright ended — switch back to siren */
            snd_state = SND_SIREN;
            siren_freq = SIREN_FREQ_LOW;
            siren_dir = 1;
            /* Reconfigure voice 0 for siren: triangle wave */
            zvb_map_peripheral(ZVB_PERI_SOUND_IDX);
            zvb_peri_sound_select = VOICE0;
            zvb_peri_sound_wave = WAV_TRIANGLE;
            zvb_peri_sound_freq_low = siren_freq & 0xFF;
            zvb_peri_sound_freq_high = (siren_freq >> 8) & 0xFF;
            zvb_peri_sound_volume = VOL_25;
        } else {
            /* Update fright pattern: ramp up, reset every 8 frames */
            fright_tick++;
            if (fright_tick >= FRIGHT_CYCLE) {
                fright_tick = 0;
                fright_freq = FRIGHT_FREQ_BASE;
            } else {
                fright_freq += FRIGHT_STEP;
            }
            zvb_map_peripheral(ZVB_PERI_SOUND_IDX);
            zvb_peri_sound_select = VOICE0;
            zvb_peri_sound_freq_low = fright_freq & 0xFF;
            zvb_peri_sound_freq_high = (fright_freq >> 8) & 0xFF;
        }
    }

    /* Update waka sound on voice 1 (independent of siren state) */
    if (waka_active) {
        waka_timer--;
        if (waka_timer == 0) {
            /* Waka finished — silence voice 1: zero freq + disable volume */
            waka_active = 0;
            zvb_map_peripheral(ZVB_PERI_SOUND_IDX);
            zvb_peri_sound_select = VOICE1;
            zvb_peri_sound_freq_low = 0;
            zvb_peri_sound_freq_high = 0;
            zvb_peri_sound_volume = VOL_0;
        } else {
            /* Update frequency: descend or ascend based on toggle */
            if (waka_toggle) {
                /* Last toggle flipped to 1, so current sound is eatdot1 (descending) */
                if (waka_freq > WAKA_STEP) {
                    waka_freq -= WAKA_STEP;
                }
            } else {
                /* Current sound is eatdot2 (ascending) */
                waka_freq += WAKA_STEP;
            }
            zvb_map_peripheral(ZVB_PERI_SOUND_IDX);
            zvb_peri_sound_select = VOICE1;
            zvb_peri_sound_freq_low = waka_freq & 0xFF;
            zvb_peri_sound_freq_high = (waka_freq >> 8) & 0xFF;
        }
    }

    /* Update ghost eaten sound on voice 2 */
    if (ghost_eat_active) {
        ghost_eat_timer--;
        if (ghost_eat_timer == 0) {
            /* Ghost eat finished — silence voice 2 */
            ghost_eat_active = 0;
            zvb_map_peripheral(ZVB_PERI_SOUND_IDX);
            zvb_peri_sound_select = VOICE2;
            zvb_peri_sound_freq_low = 0;
            zvb_peri_sound_freq_high = 0;
            zvb_peri_sound_volume = VOL_0;
        } else {
            /* Sweep frequency up */
            ghost_eat_freq += GHOST_EAT_STEP;
            zvb_map_peripheral(ZVB_PERI_SOUND_IDX);
            zvb_peri_sound_select = VOICE2;
            zvb_peri_sound_freq_low = ghost_eat_freq & 0xFF;
            zvb_peri_sound_freq_high = (ghost_eat_freq >> 8) & 0xFF;
        }
    }

    /* Update fruit eaten sound on voice 2 */
    if (fruit_snd_active && !ghost_eat_active) {
        fruit_snd_timer++;
        if (fruit_snd_timer >= FRUIT_SND_DURATION) {
            /* Done — silence voice 2 */
            fruit_snd_active = 0;
            zvb_map_peripheral(ZVB_PERI_SOUND_IDX);
            zvb_peri_sound_select = VOICE2;
            zvb_peri_sound_freq_low = 0;
            zvb_peri_sound_freq_high = 0;
            zvb_peri_sound_volume = VOL_0;
        } else {
            /* Sweep: descend for 11 ticks, then ascend */
            if (fruit_snd_timer <= 11) {
                if (fruit_snd_freq >= FRUIT_SND_FREQ_STEP) {
                    fruit_snd_freq -= FRUIT_SND_FREQ_STEP;
                } else {
                    fruit_snd_freq = 0;
                }
            } else {
                fruit_snd_freq += FRUIT_SND_FREQ_STEP;
            }
            zvb_map_peripheral(ZVB_PERI_SOUND_IDX);
            zvb_peri_sound_select = VOICE2;
            zvb_peri_sound_freq_low = fruit_snd_freq & 0xFF;
            zvb_peri_sound_freq_high = (fruit_snd_freq >> 8) & 0xFF;
        }
    }

    snd_bank_restore();
}
