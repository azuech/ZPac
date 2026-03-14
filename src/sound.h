#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

/* Initialize the PSG — call once at startup, after gfx init */
void sound_init(void);

/* Update sound effects — call once per frame at TOP of game loop,
 * right after gfx_wait_vblank(). Handles peripheral bank save/restore. */
void sound_update(void);

/* Start the background siren (normal gameplay) */
void sound_start_siren(void);

/* Silence all audio immediately (death, level complete, etc.) */
void sound_stop_all(void);

/* Trigger waka-waka sound (call when ZPac eats a dot/energizer) */
void sound_waka(void);

/* Start frightened mode sound (replaces siren on voice 0) */
void sound_start_fright(void);

/* Trigger ghost eaten sound (call when ZPac eats a frightened ghost) */
void sound_ghost_eaten(void);

/* Play the intro jingle (blocking, ~4 seconds). Call during READY! freeze. */
void sound_play_prelude(void);

/* Start death jingle — called once, then sound_death_update()
 * is called per-frame during the death animation. */
void sound_death_start(void);

/* Update death jingle for one animation frame.
 * frame = 0..11 (death animation frame index).
 * Handles frequency update and silences at end. */
void sound_death_update(uint8_t frame);

/* Stop death jingle and silence all voices */
void sound_death_stop(void);

/* Trigger fruit eaten sound (call when ZPac eats a bonus fruit) */
void sound_fruit_eaten(void);

/* Start intermission music — call once, then sound_intermission_update()
 * each frame. Returns 0 while playing, 1 when finished. */
void sound_intermission_start(void);
uint8_t sound_intermission_update(void);

/* Play coin insert sound (restores master volume + fruit sound) */
void sound_coin(void);

#endif /* SOUND_H */
