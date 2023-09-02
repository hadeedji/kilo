#ifndef KILO_H
#define KILO_H

#define KILO_VERSION "0.0.1"
#define KILO_TAB_STOP 4

#include <termios.h>
#include <time.h>

struct editor_state {
    int cx, cy, rx;
    int row_off, col_off;
    int screenrows, screencols;

    struct buffer *current_buf;
    struct termios orig_termios;

    char message[256];
    time_t message_time;
};
extern struct editor_state E;

void editor_set_message(const char *, ...);

#endif // KILO_H
