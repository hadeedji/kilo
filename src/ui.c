#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "buffer.h"
#include "erow.h"
#include "kilo.h"
#include "terminal.h"
#include "ui.h"
#include "utils.h"

static void ui_draw_rows(struct append_buf *draw_buffer);
static void ui_draw_statusbar(struct append_buf *draw_buffer);
static void ui_draw_messagebar(struct append_buf *draw_buffer);

void ui_draw_screen(void) {
    if (terminal_cursor_visibility(CURSOR_HIDDEN) == -1)
        die("term_cursor_hidden");

    struct append_buf *draw_buf = ab_create();

    ui_draw_rows(draw_buf);
    ui_draw_statusbar(draw_buf);
    ui_draw_messagebar(draw_buf);

    write(STDIN_FILENO, draw_buf->chars, draw_buf->n_chars);
    ab_free(draw_buf);

    int row_pos = E.current_buf->cy - E.current_buf->row_off + 1;
    int col_pos = E.current_buf->rx - E.current_buf->col_off + 1;
    if (terminal_set_cursor_pos(row_pos, col_pos) == -1)
        die("term_set_cursor_pos");

    if (terminal_cursor_visibility(CURSOR_SHOWN) == -1)
        die("term_cursor_hidden");
}

static void ui_draw_rows(struct append_buf *draw_buf) {
    terminal_set_cursor_pos(1, 1);
    for (int y = 0; y < E.screenrows; y++) {
        bool in_file = (y < E.current_buf->n_rows - E.current_buf->row_off);
        bool no_file = (E.current_buf->filename == NULL && E.current_buf->n_rows == 0);

        if (in_file) {
            struct erow *crow = E.current_buf->rows[y + E.current_buf->row_off];

            size_t len = crow->n_rchars - E.current_buf->col_off;
            len = MIN(len, (size_t) E.screencols);
            len = MAX(len, 0);

            ab_append(draw_buf, crow->rchars + E.current_buf->col_off, len);
        } else if (no_file && y == E.screenrows / 2) {
            char welcome[64];
            int len = snprintf(welcome, sizeof(welcome),
                               "Welcome to kilo! -- %s", STRINGIZE(KILO_COMMIT_HASH));

            int padding = (E.screencols - len) / 2;
            for (int i = 0; i < padding; i++)
                ab_append(draw_buf, (i == 0 ? "~" : " "), 1);

            ab_append(draw_buf, welcome, len);
        } else ab_append(draw_buf, "~", 1);

        ab_append(draw_buf, "\x1b[K\r\n", 5);
    }
}

static void ui_draw_statusbar(struct append_buf *draw_buf) {
    char *status_buf = malloc(E.screencols), buf[256];
    int len;

    memset(status_buf, ' ', E.screencols);

    ab_append(draw_buf, "\x1b[7m", 4);

    char *display = E.current_buf->filename ? E.current_buf->filename : "[NO NAME]";
    char *modified = E.current_buf->modified ? "(modified) " : "";
    int n_rows = E.current_buf->n_rows;
    len = sprintf(buf, "%s %s-- %d lines", display, modified, n_rows);
    memcpy(status_buf, buf, len);

    len = sprintf(buf, "%d:%d", E.current_buf->cy + 1, E.current_buf->rx + 1);
    memcpy(status_buf + E.screencols - len, buf, len);

    ab_append(draw_buf, status_buf, E.screencols);
    ab_append(draw_buf, "\x1b[m\r\n", 5);

    free(status_buf);
}

static void ui_draw_messagebar(struct append_buf *draw_buf) {
    ab_append(draw_buf, "\x1b[K", 3);

    int len = strlen(E.message);
    if (len > E.screencols) len = E.screencols;

    if (len && time(NULL) - E.message_time < 5)
        ab_append(draw_buf, E.message, len);
}
