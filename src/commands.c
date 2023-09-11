#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "commands.h"
#include "cursor.h"
#include "erow.h"
#include "input.h"
#include "kilo.h"
#include "terminal.h"
#include "utils.h"

void command_quit(void) {
    if (E.current_buf->modified && E.quit_times) {
        editor_set_message("Unsaved changed! Press quit %d more time(s)", E.quit_times);
        E.quit_times--;
    } else {
        terminal_clear();
        exit(0);
    }
}

void command_move_cursor(KEY key) {
    switch (key) {
        case ARROW_LEFT:
            cursor_move(E.current_buf, -1, 0);
            break;
        case ARROW_DOWN:
            cursor_move(E.current_buf, 0, 1);
            break;
        case ARROW_UP:
            cursor_move(E.current_buf, 0, -1);
            break;
        case ARROW_RIGHT:
            cursor_move(E.current_buf, 1, 0);
            break;

        case HOME:
            cursor_move(E.current_buf, -E.current_buf->cx, 0);
            break;
        case END:
            cursor_move(E.current_buf, buffer_get_crow_len(E.current_buf) - E.current_buf->cx, 0);
            break;

        case PG_UP:
            cursor_move(E.current_buf, 0, -E.screenrows);
            break;
        case PG_DOWN:
            cursor_move(E.current_buf, 0, E.screenrows);
            break;
    }
}

void command_insert_line(void) {
    struct erow *erow = erow_create(NULL, 0, E.current_buf);

    if (E.current_buf->cy == E.current_buf->n_rows) {
        buffer_insert_row(E.current_buf, erow, E.current_buf->cy);
    } else {
        buffer_insert_row(E.current_buf, erow, E.current_buf->cy + 1);

        struct erow *crow = buffer_get_crow(E.current_buf);

        erow_insert_chars(erow, crow->chars + E.current_buf->cx, crow->n_chars - E.current_buf->cx, 0);
        erow_delete_chars(crow, crow->n_chars - E.current_buf->cx, E.current_buf->cx);
    }

    input_process_key(ARROW_DOWN);
    input_process_key(HOME);
}

void command_insert_char(char c) {
    struct erow *erow;

    if (E.current_buf->cy == E.current_buf->n_rows) {
        erow = erow_create(NULL, 0, E.current_buf);
        buffer_insert_row(E.current_buf, erow, E.current_buf->n_rows);
    } else erow = buffer_get_crow(E.current_buf);

    erow_insert_chars(erow, &c, 1, E.current_buf->cx);
    input_process_key(ARROW_RIGHT);
}

// TODO: Is this many simulated keypresses necessary? Is it bad?
void command_delete_char(void) {
    if (E.current_buf->cy == E.current_buf->n_rows)
        input_process_key(ARROW_LEFT);

    struct erow *crow = buffer_get_crow(E.current_buf);

    if (E.current_buf->cx == 0) {
        if (E.current_buf->cy == 0)
            return;

        struct erow *prow = E.current_buf->rows[E.current_buf->cy - 1];

        input_process_key(HOME);
        input_process_key(ARROW_UP);
        cursor_move(E.current_buf, prow->n_chars, 0);

        erow_insert_chars(prow, crow->chars, crow->n_chars, prow->n_chars);
        buffer_delete_row(E.current_buf, E.current_buf->cy + 1);
    } else {
        erow_delete_chars(crow, 1, E.current_buf->cx - 1);
        input_process_key(ARROW_LEFT);
    }
}

void command_save_buffer(void) {
    if (E.current_buf->filename == NULL) {
        E.current_buf->filename = editor_prompt("Save as: %s (ESC to cancel)");
        if (E.current_buf->filename == NULL) {
            editor_set_message("Save aborted");
            return;
        }
    }

    size_t bytes_written;
    ERRCODE errcode = buffer_write_file(E.current_buf, &bytes_written);

    if (errcode == 0)
        editor_set_message("%d bytes written", bytes_written);
    else
        editor_set_message("Write error %d: %s", errcode, strerror(errno));
}
