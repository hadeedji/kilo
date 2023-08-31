#define _DEFAULT_SOURCE

#define KILO_VERSION "0.0.1"
#define KILO_TAB_STOP 4

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kilo.h"
#include "terminal.h"

#define CTRL_KEY(key) ((key) & 0x1f)

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

/*****************************************************************************/

struct string_buf;

struct EROW {
    char *chars;
    int n_chars;

    char *rchars;
    int n_rchars;
};

struct editor_state E;

/*****************************************************************************/

void editor_init(void);
void editor_open(const char *);
void editor_append_row(const char *, int);
void editor_set_message(const char *, ...);

void editor_redraw_screen(void);
void editor_draw_rows(struct string_buf *);
void editor_draw_statusbar(struct string_buf *);
void editor_draw_messagebar(struct string_buf *);

void editor_process_key(void);
void editor_move_cursor(KEY);
void editor_adjust_viewport(void);
int editor_get_rx(int);
int editor_get_cx(int);

struct string_buf *sb_init(void);
char *sb_get_string(struct string_buf *);
int sb_get_size(struct string_buf *);
void sb_append(struct string_buf *, const char *, int);
void sb_free(struct string_buf *);

/*****************************************************************************/

int main(int argc, char **argv) {
    editor_init();

    if (argc >= 2)
        editor_open(argv[1]);

    while (E.running) {
        editor_redraw_screen();
        editor_process_key();
    }

    term_clear();
    return 0;
}

/*****************************************************************************/

void editor_init(void) {
    if (!isatty(STDIN_FILENO)) {
        printf("kilo only supports a terminal at standard in. Exiting.");
        exit(1);
    }

    if (term_enable_raw() == -1) die("term_enable_raw");
    if (term_get_win_size(&E.screenrows, &E.screencols) == -1)
        die("term_get_win_size");

    E.filename = NULL;
    E.cx = E.cy = 0;
    E.rx = 0;
    E.screenrows -= 2;
    E.row_off = E.col_off = 0;
    E.running = true;

    E.rows = NULL;
    E.n_rows = 0;

    editor_set_message("Welcome to kilo. Press CTRL-Q to quit.");
}

void editor_open(const char *filename) {
    free(E.filename);
    E.filename = strdup(filename);

    FILE *file = fopen(E.filename, "r");
    if (!file) die("fopen");

    char *line = NULL;
    size_t linecap = 0;
    int linelen = -1;

    while ((linelen = getline(&line, &linecap, file)) != -1) {
        while (linelen > 0 && (line[linelen-1] == '\n' || line[linelen-1] == '\r'))
            linelen--;

        editor_append_row(line, linelen);
    }

    free(line);
    fclose(file);
}

void editor_append_row(const char *s, int len) {
    E.rows = realloc(E.rows, sizeof(struct EROW) * (E.n_rows + 1));
    struct EROW *new_row = E.rows + E.n_rows;

    new_row->chars = malloc(len);
    new_row->n_chars = len;

    struct string_buf *sb = sb_init();
    for (int i = 0; i < len; i++) {
        new_row->chars[i] = s[i];

        switch (s[i]) {
            case TAB: {
                int tabs = KILO_TAB_STOP - (sb_get_size(sb) % KILO_TAB_STOP);
                while (tabs--) sb_append(sb, " ", 1);
                break;
            }
            default:
                sb_append(sb, s+i, 1);
        }
    }

    new_row->rchars = malloc(sb_get_size(sb));
    new_row->n_rchars = sb_get_size(sb);
    memcpy(new_row->rchars, sb_get_string(sb), sb_get_size(sb));

    sb_free(sb);

    E.n_rows++;
}

void editor_set_message(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E.message, sizeof(E.message), fmt, ap);
  va_end(ap);

  E.message_time = time(NULL);
}


void editor_redraw_screen(void) {
    if (term_cursor_hidden(true) == -1)
        die("term_cursor_hidden");


    struct string_buf *draw_buf = sb_init();

    editor_draw_rows(draw_buf);
    editor_draw_statusbar(draw_buf);
    editor_draw_messagebar(draw_buf);

    write(STDIN_FILENO, sb_get_string(draw_buf), sb_get_size(draw_buf));
    sb_free(draw_buf);

    int row_pos = E.cy - E.row_off + 1;
    int col_pos = E.rx - E.col_off + 1;
    if (term_set_cursor_pos(row_pos, col_pos) == -1)
        die("term_set_cursor_pos");

    if (term_cursor_hidden(false) == -1)
        die("term_cursor_hidden");
}

void editor_draw_rows(struct string_buf *draw_buf) {
    term_set_cursor_pos(1, 1);
    for (int y = 0; y < E.screenrows; y++) {
        bool in_file = y < E.n_rows - E.row_off;
        bool no_file = E.n_rows == 0;

        if (in_file) {
            struct EROW curr_row = E.rows[y + E.row_off];
            int max_len = MIN(curr_row.n_rchars - E.col_off, E.screencols);

            sb_append(draw_buf, curr_row.rchars + E.col_off, MAX(max_len, 0));
        } else if (no_file && y == E.screenrows / 2) {
            char welcome[64];
            int len = snprintf(welcome, sizeof(welcome),
                               "Welcome to kilo! -- v%s", KILO_VERSION);

            int padding = (E.screencols - len) / 2;
            for (int i = 0; i < padding; i++)
                sb_append(draw_buf, (i == 0 ? "~" : " "), 1);

            sb_append(draw_buf, welcome, len);
        } else sb_append(draw_buf, "~", 1);

        sb_append(draw_buf, "\x1b[K\r\n", 5);
    }
}

void editor_draw_statusbar(struct string_buf *draw_buf) {
    char *status_buf = malloc(E.screencols), buf[256];
    int len;

    memset(status_buf, ' ', E.screencols);

    sb_append(draw_buf, "\x1b[7m", 4);

    len = sprintf(buf, "%s -- %d lines",
                  (E.filename ? E.filename : "[NO NAME]"), E.n_rows);
    memcpy(status_buf, buf, len);

    len = sprintf(buf, "%d:%d", E.cy + 1, E.rx + 1);
    memcpy(status_buf + E.screencols - len, buf, len);

    sb_append(draw_buf, status_buf, E.screencols);
    sb_append(draw_buf, "\x1b[m\r\n", 5);

    free(status_buf);
}
void editor_draw_messagebar(struct string_buf *draw_buf) {
    sb_append(draw_buf, "\x1b[K", 3);

    int len = strlen(E.message);
    if (len > E.screencols) len = E.screencols;

    if (len && time(NULL) - E.message_time < 5)
        sb_append(draw_buf, E.message, len);
}


void editor_process_key(void) {
    KEY c = term_read_key();

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
            E.running = false;
            break;
    }
}

void editor_move_cursor(KEY key) {
    int max_x = (E.cy < E.n_rows ? E.rows[E.cy].n_chars : 0);

    switch (key) {
        case ARROW_LEFT:
            if (E.cx > 0) E.cx--;
            else if (E.cy > 0) E.cx = E.rows[--E.cy].n_chars;
            break;
        case ARROW_DOWN:
            if (E.cy < E.n_rows) E.cy++;
            break;
        case ARROW_UP:
            if (E.cy > 0) E.cy--;
            break;
        case ARROW_RIGHT:
            if (E.cx < max_x) E.cx++;
            else if (E.cy < E.n_rows) {
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
            if ((E.cy += E.screenrows) > E.n_rows)
                E.cy = E.n_rows;
            break;
    }

    static int saved_rx = 0;
    bool horizontal = (key == ARROW_LEFT || key == ARROW_RIGHT ||
        key == HOME || key == END);
    if (horizontal) saved_rx = editor_get_rx(E.cx);
    else E.cx = editor_get_cx(saved_rx);

    max_x = (E.cy < E.n_rows ? E.rows[E.cy].n_chars : 0);
    if (E.cx > max_x)
        E.cx = max_x;

    E.rx = editor_get_rx(E.cx);
    editor_adjust_viewport();
}

void editor_adjust_viewport(void) {
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

int editor_get_rx(int cx) {
    if (E.cy >= E.n_rows) return 0;
    int rx = 0;

    struct EROW *row = E.rows+E.cy;
    for (int i = 0; i < MIN(cx, row->n_chars); i++) {
        if (row->chars[i] == TAB)
            rx += KILO_TAB_STOP - (rx % KILO_TAB_STOP);
        else rx++;
    }

    return rx;
}

int editor_get_cx(int rx_target) {
    if (E.cy >= E.n_rows) return 0;
    int cx = 0, rx = 0;

    struct EROW *row = E.rows+E.cy;
    while (rx < rx_target && cx < row->n_chars) {
        if (row->chars[cx] == TAB) {
            rx += KILO_TAB_STOP - (rx % KILO_TAB_STOP);
        } else rx++;

        cx++;
    }

    return (rx >= rx_target ? cx : row->n_chars);
}

/*****************************************************************************/

struct string_buf {
    char *chars;
    int n_chars;
};

struct string_buf *sb_init(void) {
    size_t buf_size = sizeof(struct string_buf);
    struct string_buf *sb = (struct string_buf *) malloc(buf_size);

    sb->chars = NULL;
    sb->n_chars = 0;

    return sb;
}

char *sb_get_string(struct string_buf *sb) {
    return sb->chars;
}

int sb_get_size(struct string_buf *sb) {
    return sb->n_chars;
}

void sb_append(struct string_buf *sb, const char *chars, int n_chars) {
    sb->chars = realloc(sb->chars, sb->n_chars + n_chars);

    memcpy(sb->chars + sb->n_chars, chars, n_chars);
    sb->n_chars += n_chars;
}

void sb_free(struct string_buf *sb) {
    free(sb->chars);
    free(sb);
}

/*****************************************************************************/

void die(const char *s) {
    term_clear();

    perror(s);
    exit(1);
}
