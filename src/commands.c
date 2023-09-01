#include <stdbool.h>

#include "commands.h"
#include "kilo.h"
#include "buffer.h"

void editor_move_cursor(KEY key) {
    struct erow *rows = E.current_buf->rows;
    int n_rows = E.current_buf->n_rows;

    struct erow *current_row = (E.cy < n_rows ? rows + E.cy : NULL);
    int max_x = (current_row ? current_row->n_chars : 0);

    switch (key) {
        case ARROW_LEFT:
            if (E.cx > 0) E.cx--;
            else if (E.cy > 0) E.cx = rows[--E.cy].n_chars;
            break;
        case ARROW_DOWN:
            if (E.cy < n_rows) E.cy++;
            break;
        case ARROW_UP:
            if (E.cy > 0) E.cy--;
            break;
        case ARROW_RIGHT:
            if (E.cx < max_x) E.cx++;
            else if (E.cy < n_rows) {
                E.cx = 0;
                E.cy++;
            }
            break;

        case HOME:
            E.cx = 0;
            break;
        case END:
            E.cx = max_x;
            break;

        case PG_UP:
            if ((E.cy -= E.screenrows) < 0)
                E.cy = 0;
            break;
        case PG_DOWN:
            if ((E.cy += E.screenrows) > n_rows)
                E.cy = n_rows;
            break;
    }

    current_row = (E.cy < n_rows ? rows + E.cy : NULL);
    max_x = (current_row ? current_row->n_chars : 0);

    static int saved_rx = 0;
    bool horizontal = (key == ARROW_LEFT || key == ARROW_RIGHT ||
        key == HOME || key == END);
    if (horizontal) saved_rx = erow_cx_to_rx(current_row, E.cx);
    else E.cx = erow_rx_to_cx(current_row, saved_rx);

    if (E.cx > max_x)
        E.cx = max_x;

    E.rx = erow_cx_to_rx(&E.current_buf->rows[E.cy], E.cx);

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
