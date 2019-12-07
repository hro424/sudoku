#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

struct cell {
    /* holds a number.
     * 0   -- not solved yet
     * 1-9 -- solved
     */
	int num;

    /* holds candidates.
     * bit position + 1 indicates the number.
     *
     * Each bit holds one of the following two states:
     *   0 -- the number is never set in this cell.
     *   1 -- the number may be set in this cell.
     */
	uint16_t bitmap;
};

#define SQUARE(n)           ((n) * (n))
#define BOX_EDGE_LENGTH     3
#define BOX_SIZE            SQUARE(BOX_EDGE_LENGTH)
#define BOARD_BOX_NUMBER    3
#define BOARD_EDGE_LENGTH   (BOX_EDGE_LENGTH * BOARD_BOX_NUMBER)
#define BOARD_SIZE          SQUARE(BOARD_EDGE_LENGTH)

struct board {
	struct cell cells[81];
};

/**
 * Sets the specific number to the cell.
 *
 * @param c     the cell
 * @param num   the number between 1 and 9
 */
void
cell_set(struct cell *c, int num)
{
	if (c != NULL && 0 < num && num <= 9) {
		c->num = num;
		c->bitmap = 1 << (num - 1);
	}
}

/**
 * Initializes the cell.
 * The initial state of a cell may take any number between 1 and 9.
 *
 * @param c     the cell
 */
void
cell_clear(struct cell *c)
{
    if (c != NULL) {
        c->num = 0;
        c->bitmap = (1 << 9) - 1;
    }
}

/**
 * Removes the candidates of the cell which are also in the other cell.
 *
 * @param c     the cell
 * @param ref   the other cell
 */
void
cell_unset(struct cell *c, struct cell *ref)
{
	if (c != NULL && ref != NULL & c != ref) {
		c->bitmap &= ~ref->bitmap;
	}
}

/**
 * Counts the number of 1-bits in the bitmap.
 */
static int
numbits(uint16_t bitmap)
{
	int num = 0;
	for (int i = 0; i < 16; i++) {
		num += (bitmap >> i) & 1U;
	}
	return num;
}

/**
 * Counts the number of candidates.
 */
int
cell_candidates(struct cell *c)
{
	return numbits(c->bitmap);
}

/**
 * Set the smallest candidate of the bitmap to the cell.
 */
void
cell_autoset(struct cell *c)
{
	for (int i = 0; i < 9; i++) {
		if ((c->bitmap >> i) & 1U) {
			c->num = i + 1;
		}
	}
}

void
dump(struct board *b)
{
	for (int y = 0; y < 9; y++) {
		for (int x = 0; x < 9; x++) {
			printf("%d ", b->cells[y * 9 + x].num);
		}

		printf("\t");

		for (int x = 0; x < 9; x++) {
			printf("%.4x ", b->cells[x + y * 9].bitmap);
		}

		printf("\n");
	}
	printf("\n");
}

static struct cell *
get_cell(struct board *b, int x_base, int y_base, int x, int y)
{
	return &b->cells[x_base * 3 + x + (y_base * 3 + y) * 9];
}

static void
unset_horizontal(struct board *b, struct cell *c, int y_axis)
{
	if (c->num > 0) {
		for (int x = 0; x < 9; x++) {
			struct cell *target = get_cell(b, 0, 0, x, y_axis);
			cell_unset(target, c);
		}
	}
}

static void
unset_vertical(struct board *b, struct cell *c, int x_axis)
{
	if (c->num > 0) {
		for (int y = 0; y < 9; y++) {
			struct cell *target = get_cell(b, 0, 0, x_axis, y);
			cell_unset(target, c);
		}
	}
}

void
unset_box(struct board *b, struct cell *c, int x_axis, int y_axis)
{
	int x_base = x_axis / 3;
	int y_base = y_axis / 3;

	if (c->num > 0) {
		for (int y = 0; y < 3; y++) {
			for (int x = 0; x < 3; x++) {
				struct cell *target = get_cell(b, x_base, y_base, x, y);
				cell_unset(target, c);
			}
		}
	}
}

void
make_yfilter(struct board *b, int x_base, int y_base, uint16_t *filter)
{
    uint16_t tmp[3] = {0, 0, 0};

    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            struct cell *c = get_cell(b, x_base, y_base, x, y);
            tmp[y] |= c->bitmap;
            printf("  tmp[%d, %d] = %.4X\n", x, y, c->bitmap);
        }
    }

    filter[0] = tmp[0] & ~tmp[1] & ~tmp[2];
    filter[1] = tmp[1] & ~tmp[0] & ~tmp[2];
    filter[2] = tmp[2] & ~tmp[0] & ~tmp[1];
}

void
make_xfilter(struct board *b, int x_base, int y_base, uint16_t *filter)
{
    uint16_t tmp[3] = {0, 0, 0};

    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
            struct cell *c = get_cell(b, x_base, y_base, x, y);
            tmp[x] |= c->bitmap;
        }
    }

    filter[0] = tmp[0] & ~tmp[1] & ~tmp[2];
    filter[1] = tmp[1] & ~tmp[0] & ~tmp[2];
    filter[2] = tmp[2] & ~tmp[0] & ~tmp[1];
}

void
scan(struct board *b)
{
	for (int y = 0; y < 9; y++) {
		for (int x = 0; x < 9; x++) {
			struct cell *c = get_cell(b, 0, 0, x, y);
			unset_horizontal(b, c, y);
			unset_vertical(b, c, x);
			//printf("after v/h\n");
			//dump(b);
			unset_box(b, c, x, y);
			//printf("after box\n");
			//dump(b);
		}
	}

    /*
    for (int y = 0; y < BOARD_BOX_NUMBER; y++) {
        for (int x = 0; x < BOARD_BOX_NUMBER; x++) {
            uint16_t filter[BOX_EDGE_LENGTH];
            struct cell c;

            make_xfilter(b, x, y, filter);

            for (int x_offset = 0; x_offset < BOX_EDGE_LENGTH; x_offset++) {
                //printf("xfilter[%d] = %.8X\n", x * 3 + x_offset, filter[x_offset]);
                c.bitmap = filter[x_offset];
                unset_vertical(b, &c, x * 3 + x_offset);
            }

            make_yfilter(b, x, y, filter);

            for (int y_offset = 0; y_offset < BOX_EDGE_LENGTH; y_offset++) {
                printf("yfilter[%d, %d] = %.4X\n", x, y * 3 + y_offset, filter[y_offset]);
                c.bitmap = filter[y_offset];
                unset_horizontal(b, &c, y * 3 + y_offset);
            }
        }
    }
    */
}

void
unique_box(struct board *b, int x_base, int y_base)
{
	for (int y = 0; y < 3; y++) {
		for (int x = 0; x < 3; x++) {
			struct cell *current = get_cell(b, x_base, y_base, x, y);
			if (current->num == 0) {
				uint16_t bitmap = current->bitmap;
				for (int j = 0; j < 3; j++) {
					for (int i = 0; i < 3; i++) {
						if (!(i == x && j == y)) {
							struct cell *c = get_cell(b, x_base, y_base, i, j);
							bitmap &= ~c->bitmap;
						}
					}
				}

				//printf("(%d, %d) 0x%.8X\n", x_base + x, y_base + y, bitmap);
				if (numbits(bitmap) == 1) {
					current->bitmap = bitmap;
					cell_autoset(current);
				}
			}
		}
	}
}

void
sweep(struct board *b)
{
	for (int yy = 0; yy < 3; yy++) {
		for (int xx = 0; xx < 3; xx++) {
			for (int y = 0; y < 3; y++) {
				for (int x = 0; x < 3; x++) {
					struct cell *c = get_cell(b, xx, yy, x, y);
					//printf("(%d, %d) %d\n", xx * 3 + x, yy * 3 + y, cell_candidates(c));
					if (c->num == 0 && cell_candidates(c) == 1) {
						cell_autoset(c);
					}
				}
			}
			unique_box(b, xx, yy);
		}
	}
}

bool
is_finished(struct board *b)
{
    for (int i = 0; i < 81; i++) {
        if (b->cells[i].num == 0) {
            return false;
        }
    }
    return true;
}

static int test1[81] = {
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

static int test2[81] = {
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
	struct board *board;
	board = malloc(sizeof(struct board));

	for (int i = 0; i < 81; i++) {
		cell_clear(&board->cells[i]);
		cell_set(&board->cells[i], TEST[i]);
	}

	dump(board);

	for (int i = 0; i < 4; i++) {
		scan(board);
		sweep(board);
		printf("after sweep\n");
		dump(board);
        if (is_finished(board)) {
            printf("%d iterations\n", i);
            break;
        }
	}

	free(board);
	return EXIT_SUCCESS;
}

