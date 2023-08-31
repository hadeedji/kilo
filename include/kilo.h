#ifndef KILO_H
#define KILO_H

#include <stdbool.h>
#include <stdint.h>
#include <termios.h>
#include <time.h>

struct editor_state {
    char *filename;
    int cx, cy;
    int rx;
    int screenrows, screencols;
    int row_off, col_off;
    bool running;

    struct EROW *rows;
    int n_rows;

    char message[256];
    time_t message_time;

    struct termios orig_termios;
};
extern struct editor_state E;

typedef uint16_t KEY;

void die(const char *);

enum KEYS {
    TAB = 9,
    HOME = 0x100,
    DEL,
    PG_UP,
    PG_DOWN,
    END,
    ARROW_UP,
    ARROW_DOWN,
    ARROW_LEFT,
    ARROW_RIGHT,
    NOP
};


#endif // KILO_H
