#include <stdlib.h>

#include "commands.h"
#include "input.h"
#include "kilo.h"
#include "terminal.h"
#include "utils.h"

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
            command_move_cursor(c);
            break;

        case CTRL_KEY('Q'):
            command_quit();
            return;

        case CTRL_KEY('S'):
            command_save_buffer();
            break;

        case '\r':
            // TODO
            break;

        case BACKSPACE:
        case CTRL_KEY('H'):
        case DEL:
            command_delete_char();
            break;

        case CTRL_KEY('L'):
        case NOP:
            break;

        default:
            command_insert_char(c);
    }

    E.quit_times = 3;
}
