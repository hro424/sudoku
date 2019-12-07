#include <stdio.h>
#include <stdlib.h>
#include "sudoku.h"

void
cell_clear(struct cell *cp)
{
    cp->raw = 0;
    cp->state = (1U << 9) - 1;
}

void
cell_set(struct cell *cp, int num)
{
    if (num > 0) {
        cp->state = 1U << (num - 1);
        cp->num = num;
    }
}

static size_t
numbits16(uint16_t bitmap)
{
    size_t num = 0;
    for (int i = 0; i < 16; i++) {
        num += (bitmap >> i) & 1U;
    }
    return num;
}

size_t
cell_count(struct cell *cp)
{
    return numbits16(cp->state);
}

void
cell_autoset(struct cell *cp)
{
    for (int i = 0; i < 9; i++) {
        if ((cp->state >> i) & 1) {
            cp->num = i + 1;
            break;
        }
    }
}

static void
board_iterator(struct board *bp,
               void (*func)(struct board *bp, int x_base, int y_base,
                            int x_offset, int y_offset,
                            int cell_x, int cell_y))
{
    for (int yb = 0; yb < BOARD_EDGE_BOXES; yb++) {
        for (int xb = 0; xb < BOARD_EDGE_BOXES; xb++) {
            for (int yo = 0; yo < BOX_EDGE_CELLS; yo++) {
                for (int xo = 0; xo < BOX_EDGE_CELLS; xo++) {
                    int cx = SEL(xb, xo);
                    int cy = SEL(yb, yo);
                    func(bp, xb, yb, xo, yo, cx, cy);
                }
            }
        }
    }
}

void
board_init(struct board *bp, int *data)
{
    for (int y = 0; y < BOARD_EDGE_BOXES; y++) {
        for (int x = 0; x < BOARD_EDGE_BOXES; x++) {
            for (int offset_y = 0; offset_y < BOX_EDGE_CELLS; offset_y++) {
                for (int offset_x = 0; offset_x < BOX_EDGE_CELLS; offset_x++) {
                    int xi = SEL(x, offset_x);
                    int yi = SEL(y, offset_y);
                    int num = data[xi + yi * 9];

                    //printf("%d %d %d %d %d\n", x, y, offset_x, offset_y, num);
                    cell_clear(&bp->cs[xi][yi]);
                    if (num > 0) {
                        cell_set(&bp->cs[xi][yi], num);
                    }

                    bp->bs[x][y].cs[offset_x][offset_y] = &bp->cs[xi][yi];
                }
            }
        }
    }
}

void
board_dump(struct board *bp)
{
    for (int y = 0; y < BOARD_EDGE_CELLS; y++) {
        for (int x = 0; x < BOARD_EDGE_CELLS; x++) {
            printf("%d ", bp->cs[x][y].num);
        }
        printf("\t");
        for (int x = 0; x < BOARD_EDGE_CELLS; x++) {
            printf("%.3X ", bp->cs[x][y].state);
        }
        printf("\n");
    }
    printf("\n");
}

/**
 * Eliminates the numbers in the cell on the left hand side
 * with that on the right hand side.
 */
static void
cell_eliminate(struct cell *lhs, struct cell *rhs)
{
    if (lhs != rhs) {
        lhs->state &= ~rhs->state;
    }
}

static void
eliminate_hv(struct board *bp, struct cell *cp, int x, int y)
{
    if (cp->num > 0) {
        for (int i = 0; i < BOARD_EDGE_CELLS; i++) {
            struct cell *target;
            target = &bp->cs[x][i];
            cell_eliminate(target, cp);
            target = &bp->cs[i][y];
            cell_eliminate(target, cp);
        }
    }
}

static void
eliminate_box(struct board *bp, struct cell *cp,
              int box_x, int box_y, int offset_x, int offset_y)
{
    // The given cell is finished.
    // Eliminates the number of the cell in the others.
    if (cp->num > 0) {
        for (int y = 0; y < BOX_EDGE_CELLS; y++) {
            for (int x = 0; x < BOX_EDGE_CELLS; x++) {
                struct cell *target = bp->bs[box_x][box_y].cs[x][y];
                cell_eliminate(target, cp);
            }
        }
    }
}

void
unique_box(struct board *bp, struct cell *cp, int box_x, int box_y,
           int offset_x, int offset_y)
{
    // Not finished
    if (cp->num == 0) {
        uint16_t state = cp->state;

        // scan the others and figure out unique numbers
        for (int y = 0; y < BOX_EDGE_CELLS; y++) {
            for (int x = 0; x < BOX_EDGE_CELLS; x++) {
                if (!(x == offset_x && y == offset_y)) {
                    state &= ~bp->bs[box_x][box_y].cs[x][y]->state;
                }
            }
        }

        // This cell can only hold the number.  The others cannot.
        if (numbits16(state) == 1) {
            cp->state = state;
        }
    }
}

static void
scanner(struct board *bp, int box_x, int box_y, int offset_x, int offset_y,
        int cell_x, int cell_y)
{
    struct cell *c = &bp->cs[cell_x][cell_y];

    eliminate_hv(bp, c, cell_x, cell_y);
    eliminate_box(bp, c, box_x, box_y, offset_x, offset_y);
    unique_box(bp, c, box_x, box_y, offset_x, offset_y);
}


void
board_scan(struct board *bp)
{
    board_iterator(bp, scanner);
}

static void
sweeper(struct board *bp, int box_x, int box_y, int offset_x, int offset_y,
        int cell_x, int cell_y)
{
    struct cell *c = &bp->cs[cell_x][cell_y];
    if (c->num == 0 && cell_count(c) == 1) {
        cell_autoset(c);
    }
}

void
board_sweep(struct board *bp)
{
    board_iterator(bp, sweeper);
}

bool
board_is_finished(struct board *bp)
{
    for (int y = 0; y < BOARD_EDGE_CELLS; y++) {
        for (int x = 0; x < BOARD_EDGE_CELLS; x++) {
            if (bp->cs[x][y].num == 0) {
                return false;
            }
        }
    }
    return true;
}

static int test1[BOARD_SIZE] = {
	0, 0, 2, 0, 6, 4, 0, 9, 0,
	5, 0, 0, 0, 9, 0, 6, 0, 0,
	0, 3, 0, 0, 0, 0, 0, 0, 7,
	1, 0, 0, 0, 3, 0, 0, 0, 0,
	3, 8, 0, 9, 0, 6, 0, 4, 1,
	0, 0, 0, 0, 7, 0, 0, 0, 2,
	2, 0, 0, 0, 0, 0, 0, 3, 0,
	0, 0, 3, 0, 5, 0, 0, 0, 6,
	0, 5, 0, 2, 1, 0, 9, 0, 0
};

static int test2[BOARD_SIZE] = {
    0, 0, 1, 4, 0, 7, 0, 0, 2,
    0, 0, 0, 0, 0, 0, 5, 7, 0,
    0, 3, 0, 0, 0, 0, 4, 0, 9,
    0, 6, 9, 0, 0, 2, 0, 0, 0,
    4, 0, 7, 0, 0, 0, 1, 0, 5,
    0, 0, 0, 7, 0, 0, 8, 2, 0,
    1, 0, 4, 0, 0, 0, 0, 6, 0,
    0, 9, 3, 0, 0, 0, 0, 0, 0,
    5, 0, 0, 9, 0, 3, 7, 0, 0
};

#define TEST    test1

int
main(int argc, char *argv[])
{
    struct board b;

    board_init(&b, TEST);
    board_dump(&b);

    for (int i = 0; i < 16; i++) {
        board_scan(&b);
        board_sweep(&b);
        board_dump(&b);
        if (board_is_finished(&b)) {
            printf("%d iterations.\n", i);
            break;
        }
    }

    return EXIT_SUCCESS;
}
