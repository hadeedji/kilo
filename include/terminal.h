#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdbool.h>

#include "kilo.h"

int term_enable_raw(void);
void term_disable_raw(void);
int term_clear(void);
int term_cursor_hidden(bool);
int term_get_win_size(int *, int *);
int term_get_cursor_pos(int *, int *);
int term_set_cursor_pos(int, int);
KEY term_read_key(void);

#endif // TERMINAL_H
