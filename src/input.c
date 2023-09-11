#include "commands.h"
#include "input.h"
#include "kilo.h"

void input_process_key(KEY c) {
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

        case CTRL_KEY('S'):
            command_save_buffer();
            break;

        case ENTER:
            command_insert_line();
            break;

        case DEL:
            input_process_key(ARROW_RIGHT);
        case BACKSPACE:
        case CTRL_KEY('H'):
            command_delete_char();
            break;

        case CTRL_KEY('L'):
        case ESCAPE:
        case NOP:
            break;

        default:
            command_insert_char(c);
            break;

        case CTRL_KEY('Q'):
            command_quit();
            return;
    }

    E.quit_times = 3;
}
