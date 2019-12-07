#ifndef SUDOKU_SUDOKU_H
#define SUDOKU_SUDOKU_H

#include <stdbool.h>
#include <stdint.h>

#define SQUARE(n)           ((n) * (n))
#define BOX_EDGE_CELLS      3
#define BOX_SIZE            SQUARE(BOX_EDGE_CELLS)
#define BOARD_EDGE_BOXES    3
#define BOARD_EDGE_CELLS    (BOX_EDGE_CELLS * BOARD_EDGE_BOXES)
#define BOARD_SIZE          SQUARE(BOARD_EDGE_CELLS)

#define INLINE              static inline

struct cell {
    union {
        uint16_t raw;
        struct {
            unsigned int state:9;
            unsigned int reserved:3;
            unsigned int num:4;
        };
    };
};

struct box {
    struct cell *cs[BOX_EDGE_CELLS][BOX_EDGE_CELLS];
};

struct board {
    struct cell cs[BOARD_EDGE_CELLS][BOARD_EDGE_CELLS];
    struct box bs[BOARD_EDGE_BOXES][BOARD_EDGE_BOXES];
};

#define SEL(base, offset)   ((base) * 3 + (offset))

void cell_init(struct cell *cp, int num);
void cell_set(struct cell *cp, int num);
size_t cell_count(struct cell *cp);

void board_init(struct board *bp, int *data);
void board_scan(struct board *bp);
void board_sweep(struct board *bp);
void board_dump(struct board *bp);
bool board_is_finished(struct board *bp);

#endif /* SUDOKU_SUDOKU_H */

