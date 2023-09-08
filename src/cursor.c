#include "buffer.h"
#include "cursor.h"
#include "erow.h"
#include "kilo.h"
#include "utils.h"

static void cursor_adjust_viewport(struct buffer *buffer);

void cursor_move(struct buffer *buffer, int dx, int dy) {
    static int saved_rx = 0;

    buffer->cy = CLAMP(buffer->cy + dy, 0, buffer->n_rows);

    if (dx == 0)
        E.current_buf->cx = erow_rx_to_cx(buffer_get_crow(E.current_buf), saved_rx);

    int new_x = buffer->cx + dx, max_x = buffer_get_crow_len(buffer);
    if (new_x < 0) {
        if (buffer->cy > 0) {
            buffer->cy--;
            buffer->cx = buffer_get_crow_len(buffer);
        } else buffer->cx = 0;
    } else if (new_x > max_x) {
        if (buffer->cy < buffer->n_rows) {
            buffer->cy++;
            buffer->cx = 0;
        } else buffer->cx = max_x;
    } else buffer->cx = new_x;

    cursor_adjust_viewport(buffer);
    buffer->rx = erow_cx_to_rx(buffer_get_crow(buffer), buffer->cx);

    if (dy == 0)
        saved_rx = buffer->rx;
}

static void cursor_adjust_viewport(struct buffer *buffer) {
    int min_row_off = buffer->cy - (E.screenrows - 1);
    int max_row_off = buffer->cy;
    buffer->row_off = CLAMP(buffer->row_off, min_row_off, max_row_off);

    int min_col_off = buffer->rx - (E.screencols - 1);
    int max_col_off = buffer->rx;
    buffer->col_off = CLAMP(buffer->col_off, min_col_off, max_col_off);
}
