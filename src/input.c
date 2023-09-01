#include <stdlib.h>

#include "input.h"
#include "terminal.h"
#include "commands.h"

void input_process_key(void) {
    KEY c = terminal_read_key();

    switch (c) {
        case ARROW_LEFT:
        case ARROW_DOWN:
        case ARROW_UP:
        case ARROW_RIGHT:
        case HOME:
        case END:
        case PG_UP:
        case PG_DOWN:
            editor_move_cursor(c);
            break;

        case CTRL_KEY('Q'):
            terminal_clear();
            exit(0);
            break;
    }
}
