#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "commands.h"
#include "input.h"
#include "kilo.h"
#include "terminal.h"
#include "utils.h"

static void cursor_adjust_viewport(void);
static void cursor_check_file_bounds(bool horizontal);
static void cursor_update_rx(bool horizontal);

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
    struct erow *rows = E.current_buf->rows;
    int n_rows = E.current_buf->n_rows;

    struct erow *erow = (rows && E.cy < n_rows ? rows + E.cy : NULL);
    int max_x = (erow ? erow->n_chars : 0);

    switch (key) {
        case ARROW_LEFT:
            E.cx--;
            break;
        case ARROW_DOWN:
            E.cy++;
            break;
        case ARROW_UP:
            E.cy--;
            break;
        case ARROW_RIGHT:
            E.cx++;
            break;

        case HOME:
            E.cx = 0;
            break;
        case END:
            E.cx = max_x;
            break;

        case PG_UP:
            E.cy -= E.screenrows;
            break;
        case PG_DOWN:
            E.cy += E.screenrows;
            break;
    }

    bool horizontal = (key == ARROW_LEFT  || 
        key == ARROW_RIGHT ||
        key == HOME        ||
        key == END);

    cursor_check_file_bounds(horizontal);
    cursor_update_rx(horizontal);
    cursor_adjust_viewport();
}

// TODO: Improve this
void command_insert_line(void) {
    if (E.cy == E.current_buf->n_rows) {
        buffer_insert_row(E.current_buf, NULL, 0, E.cy);
    } else {
        buffer_insert_row(E.current_buf, NULL, 0, E.cy + 1);

        struct erow *c_row = E.current_buf->rows + E.cy;
        struct erow *n_row = E.current_buf->rows + E.cy + 1;

        erow_append_string(n_row, c_row->chars + E.cx, c_row->n_chars - E.cx);

        c_row->chars = realloc(c_row->chars, E.cx);
        c_row->n_chars = E.cx;
        erow_update_rendering(c_row);
    }

    E.cx = E.rx = 0;
    E.cy++;

    cursor_adjust_viewport();
}

void command_insert_char(char c) {
    if (E.cy == E.current_buf->n_rows)
        buffer_insert_row(E.current_buf, NULL, 0, E.current_buf->n_rows);

    erow_insert_char(E.current_buf->rows+E.cy, E.cx++, c);
    E.rx = erow_cx_to_rx(E.current_buf->rows+E.cy, E.cx);

    cursor_adjust_viewport();
}

void command_delete_char(void) { // TODO
    if (E.cx == 0 && E.cy == 0) return;
    if (E.cy == E.current_buf->n_rows) {
        E.cx = E.current_buf->rows[--E.cy].n_chars;
        E.rx = erow_cx_to_rx(E.current_buf->rows+E.cy, E.cx);
        return;
    }

    if (E.cx > 0) {
        erow_delete_char(E.current_buf->rows+E.cy, E.cx - 1);
    }
    else {
        struct erow *c_row = E.current_buf->rows + E.cy;
        struct erow *p_row = E.current_buf->rows + E.cy - 1;

        E.cx = p_row->n_chars;

        erow_append_string(p_row, c_row->chars, c_row->n_chars);
        buffer_delete_row(E.current_buf, E.cy);

        E.cy--;
    }

    E.rx = erow_cx_to_rx(E.current_buf->rows+E.cy, E.cx);

    cursor_adjust_viewport();
}

void command_save_buffer(void) {
    if (E.current_buf->filename == NULL) {
        E.current_buf->filename = editor_prompt("Save as: %s (ESC to cancel)");
        if (E.current_buf->filename == NULL) {
            editor_set_message("Save aborted");
            return;
        }
    }

    int bytes_written;
    ERRCODE errcode = buffer_write_file(E.current_buf, &bytes_written);

    if (errcode == 0)
        editor_set_message("%d bytes written", bytes_written);
    else
        editor_set_message("Save failed: ERRCODE %d: %s", errcode, strerror(errno));
}

static void cursor_check_file_bounds(bool horizontal) {
    struct erow *rows = E.current_buf->rows;
    int n_rows = E.current_buf->n_rows;

    struct erow *erow = (rows && E.cy < n_rows ? rows + E.cy : NULL);
    int max_x = (erow ? erow->n_chars : 0);

    if (E.cy < 0)
        E.cy = 0;

    if (E.cy > n_rows)
        E.cy = n_rows;

    if (E.cx < 0) {
        if (E.cy > 0 && horizontal)
        {
            erow = &rows[--E.cy];
            max_x = erow->n_chars;

            E.cx = max_x;
        }
        else E.cx = 0;
    }

    if (E.cx > max_x) {
        if (E.cy < n_rows && horizontal) {
            E.cx = 0;
            E.cy++;
        } else E.cx = max_x;
    }
}

static void cursor_update_rx(bool horizontal) {
    struct erow *rows = E.current_buf->rows;
    int n_rows = E.current_buf->n_rows;

    struct erow *erow = (rows && E.cy < n_rows ? rows + E.cy : NULL);
    int max_x = (erow ? erow->n_chars : 0);

    static int saved_rx = 0;
    if (horizontal) saved_rx = erow_cx_to_rx(erow, E.cx);
    else {
        E.cx = erow_rx_to_cx(erow, saved_rx);
        if (E.cx > max_x)
            E.cx = max_x;
    }

    E.rx = erow_cx_to_rx(rows + E.cy, E.cx);
}

static void cursor_adjust_viewport(void) {
    int max_row_off = E.cy;
    if (E.row_off > max_row_off)
        E.row_off = max_row_off;

    int max_col_off = E.rx;
    if (E.col_off > max_col_off)
        E.col_off = max_col_off;

    int min_row_off = E.cy - (E.screenrows - 1);
    if (E.row_off < min_row_off)
        E.row_off = min_row_off;

    int min_col_off = E.rx - (E.screencols - 1);
    if (E.col_off < min_col_off)
        E.col_off = min_col_off;
}
