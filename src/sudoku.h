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

INLINE void
cell_clear(struct cell *cp)
{
    cp->raw = 0;
    cp->state = (1U << 9) - 1;
}

INLINE bool
cell_is_finished(const struct cell *cp)
{
    return (0 < cp->num);
}

INLINE void
cell_set(struct cell *cp, unsigned int num)
{
    cp->state = 1U << (num - 1);
    cp->num = num;
}

INLINE size_t
numbits16(uint16_t bitmap)
{
    size_t num = 0;
    for (int i = 0; i < 16; i++) {
        num += (bitmap >> i) & 1U;
    }
    return num;
}

INLINE size_t
cell_count(const struct cell *cp)
{
    return numbits16(cp->state);
}

INLINE void
cell_autoset(struct cell *cp)
{
    for (int i = 0; i < 9; i++) {
        if ((cp->state >> i) & 1) {
            cp->num = i + 1;
            break;
        }
    }
}

struct box {
    struct cell *cs[BOX_EDGE_CELLS][BOX_EDGE_CELLS];
};

struct board {
    struct cell cs[BOARD_EDGE_CELLS][BOARD_EDGE_CELLS];
    struct box bs[BOARD_EDGE_BOXES][BOARD_EDGE_BOXES];
};

void board_init(struct board *bp, int *data);
void board_dump(const struct board *bp);
bool board_solve(struct board *bp);

#endif /* SUDOKU_SUDOKU_H */

