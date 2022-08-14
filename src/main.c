#include <stdio.h>
#include <stdlib.h>
#include "sudoku.h"

static int
load_file(FILE *fp, struct board *bp)
{
    int *buf = malloc(sizeof(int) * BOARD_SIZE);
    int *cur = buf;
    int c;
    int num = 0;

    if (buf == NULL) {
        return -1;
    }

    while ((c = fgetc(fp)) != EOF) {
        switch (c) {
            case ',':
                *cur = num;
                num = 0;
                cur++;
                break;
            case '0': case '1': case '2': case '3': case '4': case '5':
            case '6': case '7': case' 8': case '9':
                num = num * 10 + (c - '0');
                break;
            default:
                // do nothing
                break;
        }
    }

    *cur = num;
    num = 0;
    cur++;

    if (cur - buf != BOARD_SIZE) {
        free(buf);
        return -1;
    }

    board_init(bp, buf);
    free(buf);

    return 0;
}


int
main(int argc, char *argv[])
{
    struct board b;
    int ret;
    FILE *fp;

    if (argc != 2) {
        (void)fprintf(stderr, "usage: sudoku <file>\n");
        return EXIT_FAILURE;
    }

    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        return EXIT_FAILURE;
    }

    ret = load_file(fp, &b);
    (void)fclose(fp);

    if (ret) {
        (void)fprintf(stderr, "illegal syntax\n");
        return EXIT_FAILURE;
    }

    board_dump(&b);
    (void)board_solve(&b);
    board_dump(&b);

    return EXIT_SUCCESS;
}

