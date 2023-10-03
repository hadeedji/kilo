#ifndef TERMINAL_H
#define TERMINAL_H

#include "input.h"
#include "utils.h"

ERRCODE terminal_enable_raw(void);
ERRCODE terminal_clear(void);

enum cursor_visibility {
    CURSOR_SHOWN = 0,
    CURSOR_HIDDEN = 1
};
ERRCODE terminal_cursor_visibility(enum cursor_visibility visibility);
ERRCODE terminal_get_win_size(int *rows, int *cols);

ERRCODE terminal_get_cursor_pos(int *row, int *col);
ERRCODE terminal_set_cursor_pos(int, int);

KEY terminal_read_key(void);

#endif // TERMINAL_H
