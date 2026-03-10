/* maze_logic.c — ZPac 28x31 logic map */
#include "maze_logic.h"
#include "zpac_types.h"

const int8_t dir_dx[4] = { 1,  0, -1,  0 };
const int8_t dir_dy[4] = { 0,  1,  0, -1 };

uint8_t maze_logic[31][28];
uint8_t dot_count;

/* ASCII representation of the 28x31 maze layout */
static const char maze_ascii[31][29] = {
    "############################",   /* row  0 */
    "#............##............#",   /* row  1 */
    "#.####.#####.##.#####.####.#",   /* row  2 */
    "#o####.#####.##.#####.####o#",   /* row  3  energizer */
    "#.####.#####.##.#####.####.#",   /* row  4 */
    "#..........................#",   /* row  5  26 dots */
    "#.####.##.########.##.####.#",   /* row  6 */
    "#.####.##.########.##.####.#",   /* row  7 */
    "#......##....##....##......#",   /* row  8 */
    "######.##### ## #####.######",   /* row  9 */
    "######.##### ## #####.######",   /* row 10 */
    "######.##          ##.######",   /* row 11 */
    "######.## ###--### ##.######",   /* row 12  ghost door */
    "######.## #      # ##.######",   /* row 13  ghost house */
    "      .   #      #   .      ",   /* row 14  tunnel */
    "######.## #      # ##.######",   /* row 15  ghost house */
    "######.## ######## ##.######",   /* row 16 */
    "######.##          ##.######",   /* row 17 */
    "######.## ######## ##.######",   /* row 18 */
    "######.## ######## ##.######",   /* row 19 */
    "#............##............#",   /* row 20 */
    "#.####.#####.##.#####.####.#",   /* row 21 */
    "#.####.#####.##.#####.####.#",   /* row 22 */
    "#o..##.......  .......##..o#",   /* row 23  energizer */
    "###.##.##.########.##.##.###",   /* row 24 */
    "###.##.##.########.##.##.###",   /* row 25 */
    "#......##....##....##......#",   /* row 26 */
    "#.##########.##.##########.#",   /* row 27 */
    "#.##########.##.##########.#",   /* row 28 */
    "#..........................#",   /* row 29 */
    "############################"    /* row 30 */
};

void init_maze_logic(void)
{
    uint8_t r, c;
    dot_count = 0;

    /* 1. Parse ASCII map */
    for (r = 0; r < 31; r++) {
        for (c = 0; c < 28; c++) {
            char ch = maze_ascii[r][c];
            switch (ch) {
                case '.':
                    maze_logic[r][c] = CELL_WD;
                    dot_count++;
                    break;
                case 'o':
                    maze_logic[r][c] = CELL_WE;
                    dot_count++;
                    break;
                case ' ':
                    maze_logic[r][c] = CELL_W;
                    break;
                case '-':
                    maze_logic[r][c] = CELL_WALKABLE | CELL_GHOST_HOUSE;
                    break;
                default: /* '#', anything else */
                    maze_logic[r][c] = CELL_WALL;
                    break;
            }
        }
    }

    /* 2a. TUNNEL flag: row 14, cols 0-5 and 22-27 */
    for (c = 0; c < 6; c++) {
        maze_logic[14][c] |= CELL_TUNNEL;
        maze_logic[14][27 - c] |= CELL_TUNNEL;
    }

    /* 2b. GHOST_HOUSE flag: rows 13-15, cols 11-16 (interior) */
    for (r = 13; r <= 15; r++) {
        for (c = 11; c <= 16; c++) {
            maze_logic[r][c] |= CELL_GHOST_HOUSE;
        }
    }

    /* 2c. NO_UP flag: rows 9-11, cols 12-15 (above ghost house) */
    for (r = 9; r <= 11; r++) {
        for (c = 12; c <= 15; c++) {
            if (maze_logic[r][c] & CELL_WALKABLE) {
                maze_logic[r][c] |= CELL_NO_UP;
            }
        }
    }
}
