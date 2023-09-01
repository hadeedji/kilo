#ifndef TERMINAL_H
#define TERMINAL_H

#include "input.h"
#include "utils.h"

// Functions to perform low level terminal operations, using escape codes

ERRCODE terminal_enable_raw(void);
void terminal_disable_raw(void);

// Cursor positioned at the start
ERRCODE terminal_clear(void);

// Hide or show the cursor
enum cursor_visibility {
    CURSOR_SHOWN = 0,
    CURSOR_HIDDEN = 1
};
ERRCODE terminal_cursor_visibility(enum cursor_visibility visibility);

// Calculate the current window size
ERRCODE terminal_get_win_size(int *rows, int *cols);

// Get and set the cursor position, 1-based indexing
ERRCODE terminal_get_cursor_pos(int *row, int *col);
ERRCODE terminal_set_cursor_pos(int, int);

// Read input from the terminal and parse it into a KEY
KEY terminal_read_key(void);

#endif // TERMINAL_H
