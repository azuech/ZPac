#ifndef GAME_LOOP_H
#define GAME_LOOP_H

/* Game states */
#define GAME_STATE_TITLE    0
#define GAME_STATE_PLAYING  1
#define GAME_STATE_GAMEOVER 2
#define GAME_STATE_DEMO     3

/* Start the main game loop (never returns) */
void game_loop_run(void);

#endif
