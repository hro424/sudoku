#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sudoku.h"

INLINE size_t
box_select(size_t base, size_t offset)
{
    return base * BOX_EDGE_CELLS + offset;
}

static bool
board_iterator(struct board *bp,
               bool (*func)(struct board *bp,
                            size_t x_base, size_t y_base,
                            size_t x_offset, size_t y_offset,
                            size_t cell_x, size_t cell_y))
{
    for (size_t yb = 0; yb < BOARD_EDGE_BOXES; yb++) {
        for (size_t xb = 0; xb < BOARD_EDGE_BOXES; xb++) {
            for (size_t yo = 0; yo < BOX_EDGE_CELLS; yo++) {
                for (size_t xo = 0; xo < BOX_EDGE_CELLS; xo++) {
                    size_t cx = box_select(xb, xo);
                    size_t cy = box_select(yb, yo);
                    if (!func(bp, xb, yb, xo, yo, cx, cy)) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

void
board_init(struct board *bp, int *data)
{
    for (size_t y = 0; y < BOARD_EDGE_BOXES; y++) {
        for (size_t x = 0; x < BOARD_EDGE_BOXES; x++) {
            for (size_t offset_y = 0; offset_y < BOX_EDGE_CELLS; offset_y++) {
                for (size_t offset_x = 0; offset_x < BOX_EDGE_CELLS; offset_x++) {
                    size_t xi = box_select(x, offset_x);
                    size_t yi = box_select(y, offset_y);
                    size_t num = data[xi + yi * 9];

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
board_dump(const struct board *bp)
{
    for (size_t y = 0; y < BOARD_EDGE_BOXES; y++) {
        printf("+-------+-------+-------+\t"
               "+-------------+-------------+-------------+\n");
        for (size_t yy = 0; yy < BOX_EDGE_CELLS; yy++) {
            size_t yyy = y * BOX_EDGE_CELLS + yy;
            for (size_t x = 0; x < BOARD_EDGE_BOXES; x++) {
                printf("| ");
                for (size_t xx = 0; xx < BOX_EDGE_CELLS; xx++) {
                    printf("%d ", bp->cs[x * BOX_EDGE_CELLS + xx][yyy].num);
                }
            }
            printf("|\t");
            for (size_t x = 0; x < BOARD_EDGE_BOXES; x++) {
                printf("| ");
                for (size_t xx = 0; xx < BOX_EDGE_CELLS; xx++) {
                    printf("%.3X ", bp->cs[x * BOX_EDGE_CELLS + xx][yyy].state);
                }
            }
            printf("|\n");
        }
    }
    printf("+-------+-------+-------+\t"
           "+-------------+-------------+-------------+\n\n");
}

/**
 * Eliminates the numbers in the cell on the left hand side
 * with that on the right hand side.
 */
INLINE bool
cell_eliminate(struct cell *lhs, const struct cell *rhs)
{
    lhs->state &= ~rhs->state;

    return lhs->state > 0;
}

/**
 * Eliminates the number of the finished cell.
 * NOTE: Apply only on the finished cell.
 */
static bool
eliminate_hv(struct board *bp, struct cell *cp, size_t x, size_t y)
{
    bool result = true;

    // Eliminate the number in the other cells.
    for (size_t i = 0; i < BOARD_EDGE_CELLS; i++) {
        struct cell *target;

        if (i != y) {
            target = &bp->cs[x][i];
            if (!cell_eliminate(target, cp)) {
                result = false;
                break;
            }
        }

        if (i != x) {
            target = &bp->cs[i][y];
            if (!cell_eliminate(target, cp)) {
                result = false;
                break;
            }
        }
    }

    return result;
}

/**
 * Eliminates the number of the finished cell.
 * NOTE: Apply only on the finished cell.
 */
static bool
eliminate_box(struct board *bp, struct cell *cp,
              size_t box_x, size_t box_y,
              size_t offset_x, size_t offset_y)
{
    bool result = true;

    for (size_t y = 0; y < BOX_EDGE_CELLS; y++) {
        for (size_t x = 0; x < BOX_EDGE_CELLS; x++) {
            if (!(x == offset_x && y == offset_y)) {
                struct cell *target = bp->bs[box_x][box_y].cs[x][y];
                if (!cell_eliminate(target, cp)) {
                    result = false;
                    break;
                }
            }
        }
    }

    return result;
}

/**
 * NOTE: Apply only on the undermined cell.
 *
 * @param bp        the board
 * @param cp        the current cell
 * @param box_x     the position of the box in the board
 * @param box_y     the position of the box in the board
 * @param offset_x  the position of the cell on the board
 * @param offset_y  the position of the cell on ther board
 */
static void
unique_box(struct board *bp, struct cell *cp,
           size_t box_x, size_t box_y,
           size_t offset_x, size_t offset_y)
{
    uint16_t state = cp->state;

    // scan the others and figure out unique numbers
    for (size_t y = 0; y < BOX_EDGE_CELLS; y++) {
        for (size_t x = 0; x < BOX_EDGE_CELLS; x++) {
            // Except for myself
            if (!(x == offset_x && y == offset_y)) {
                state &= ~bp->bs[box_x][box_y].cs[x][y]->state;
            }
        }
    }

    // No one but this cell can hold the number.
    // Finishing this cell on sweep.
    if (numbits16(state) == 1) {
        cp->state = state;
    }
}

static bool
scanner(struct board *bp, size_t box_x, size_t box_y,
        size_t offset_x, size_t offset_y,
        size_t cell_x, size_t cell_y)
{
    bool result = false;
    struct cell *c = &bp->cs[cell_x][cell_y];

    if (cell_is_finished(c)) {
        if (eliminate_hv(bp, c, cell_x, cell_y) &&
            eliminate_box(bp, c, box_x, box_y, offset_x, offset_y)) {
            result = true;
        }
    }
    else {
        unique_box(bp, c, box_x, box_y, offset_x, offset_y);
        result = true;
    }

    return result;
}


INLINE bool
board_scan(struct board *bp)
{
    return board_iterator(bp, scanner);
}

static bool
sweeper(struct board *bp, size_t box_x, size_t box_y,
        size_t offset_x, size_t offset_y,
        size_t cell_x, size_t cell_y)
{
    struct cell *c = &bp->cs[cell_x][cell_y];
    if (c->num == 0 && cell_count(c) == 1) {
        cell_autoset(c);
    }
    return true;
}

INLINE bool
board_sweep(struct board *bp)
{
    return board_iterator(bp, sweeper);
}

struct cell *
board_get_undetermined(struct board *bp)
{
    for (size_t y = 0; y < BOARD_EDGE_CELLS; y++) {
        for (size_t x = 0; x < BOARD_EDGE_CELLS; x++) {
            struct cell *cp = &bp->cs[x][y];
            if (cp->num == 0) {
                return cp;
            }
        }
    }
    return NULL;
}

INLINE bool
board_is_finished(const struct board *bp)
{
    return board_get_undetermined((struct board *)bp) == NULL;
}

static bool
board_is_inprogress(const struct board *lhs, const struct board *rhs)
{
    bool result = false;
    for (size_t i = 0; i < BOARD_EDGE_CELLS; i++) {
        if (memcmp(lhs->cs[i], rhs->cs[i], sizeof(struct cell) * BOARD_EDGE_CELLS) != 0) {
            result = true;
            break;
        }
    }
    return result;
}

static void
board_copy(struct board *dest, const struct board *src)
{
    for (size_t i = 0; i < BOARD_EDGE_CELLS; i++) {
        memcpy(dest->cs[i], src->cs[i], sizeof(struct cell) * BOARD_EDGE_CELLS);
    }
}

bool
board_solve(struct board *bp)
{
    bool result = true;
    struct board *backup = malloc(sizeof(struct board));
    if (backup == NULL) {
        return false;
    }

    do {
        board_copy(backup, bp);

        if (!board_scan(bp) || !board_sweep(bp)) {
            result = false;
            goto exit;
        }

        //board_dump(bp);
    } while (board_is_inprogress(bp, backup));

    if (!board_is_finished(bp)) {
        struct cell *cp = board_get_undetermined(bp);

        for (size_t i = 0; i < 9; i++) {
            if (cp->state & (1 << i)) {
                cell_set(cp, i + 1);
                result = board_solve(bp);
                if (result) {
                    break;
                }
                else {
                    board_copy(bp, backup);
                }
            }
        }
    }

exit:
    free(backup);
    return result;
}


