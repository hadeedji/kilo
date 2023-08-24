#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

struct abuf {
    char *p;
    int len;
};

struct {
    struct termios orig_termios;
    int rows;
    int cols;
    int cx, cy;
} E = { .cx = 0, .cy = 0 };

enum keys {
    ARROW_UP = 0x100 + 1,
    ARROW_DOWN,
    ARROW_LEFT,
    ARROW_RIGHT
};

#define ABUF_INIT {NULL, 0}

void ab_append(struct abuf *, const char *);
void ab_free(struct abuf *);
void ab_write(struct abuf *);

void enable_raw_mode();
void disable_raw_mode();

int get_window_size(int *, int *);
int read_cursor_position(int *, int *);

void editor_init();
void editor_redraw_screen();
void editor_clear_screen();
void editor_draw_rows(struct abuf *);
void editor_process_key();
int editor_read_key();

void die(const char *);

#define CTRL_KEY(key) ((key) & 0x1f)

int main() {
    editor_init();

    while (1) {
        editor_redraw_screen();
        editor_process_key();
    }

    return 0;
}

void editor_init() {
    if (!isatty(STDIN_FILENO)) {
        printf("kilo only supports a terminal at standard in. Exiting.");
        exit(1);
    }

    enable_raw_mode();

    if (get_window_size(&E.rows, &E.cols) == -1) die("get_window_size");
}

void enable_raw_mode() {
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disable_raw_mode);

    struct termios raw = E.orig_termios;
    cfmakeraw(&raw);

    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

void disable_raw_mode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}

int get_window_size(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1 && ws.ws_col != 0) {
        *rows = ws.ws_row;
        *cols = ws.ws_col;

        return 0;
    } else {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;

        return read_cursor_position(rows, cols);
    }

    return -1;
}

int read_cursor_position(int *rows, int *cols) {
    char buf[32];
    int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    if (read(STDIN_FILENO, buf, 1) != 1 && *buf != '\x1b') return -1;
    if (read(STDIN_FILENO, buf, 1) != 1 && *buf != '[') return -1;

    do {
        if (i == (int) sizeof(buf)) return -1;
        if (read(STDIN_FILENO, buf+i, 1) == 0) break;
    } while(buf[i++] != 'R');
    buf[i-1] = '\0';

    if (sscanf(buf, "%d;%d", rows, cols) != 2) return -1;

    return 0;
}

void editor_redraw_screen() {
    struct abuf ab = ABUF_INIT;

    ab_append(&ab, "\x1b[?25l");
    ab_append(&ab, "\x1b[H");

    editor_draw_rows(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
    ab_append(&ab, buf);

    ab_append(&ab, "\x1b[?25h");

    ab_write(&ab);
    ab_free(&ab);
}

void editor_clear_screen() {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
}

void editor_draw_rows(struct abuf *ab) {
    for (int y = 0; y < E.rows; y++) {
        ab_append(ab, "~");

        ab_append(ab, "\x1b[K");
        if (y < E.rows - 1) ab_append(ab, "\r\n");
    }
}

void editor_process_key() {
    int c = editor_read_key();

    switch (c) {
        case CTRL_KEY('Q'):
            editor_clear_screen();
            exit(0);
            break;

        case ARROW_LEFT:
            if (E.cx > 0) E.cx--;
            break;
        case ARROW_DOWN:
            if (E.cy < E.rows - 1) E.cy++;
            break;
        case ARROW_UP:
            if (E.cy > 0) E.cy--;
            break;
        case ARROW_RIGHT:
            if (E.cx < E.cols - 1) E.cx++;
            break;
        default:
            printf("%d\r\n", c);
            break;
    }
}

int editor_read_key() {
    char c;
    while (read(STDIN_FILENO, &c, 1) == 0);

    if (c == '\x1b') {
        char buf[2];

        if (read(STDIN_FILENO, buf+0, 1) != 1) return c;
        if (read(STDIN_FILENO, buf+1, 1) != 1) return c;

        if (buf[0] == '[') {
            switch (buf[1]) {
                case 'A':
                    return ARROW_UP;
                    break;
                case 'B':
                    return ARROW_DOWN;
                    break;
                case 'C':
                    return ARROW_RIGHT;
                    break;
                case 'D':
                    return ARROW_LEFT;
                    break;
            }
        }
    }

    return (int) c;
}

void die(const char *s) {
    editor_clear_screen();

    perror(s);
    exit(1);
}

void ab_append(struct abuf *ab, const char *s) {
    int len = strlen(s);

    ab->p = realloc(ab->p, ab->len + len);
    memcpy(ab->p + ab->len, s, len);

    ab->len += len;
}

void ab_write(struct abuf *ab) {
    write(STDOUT_FILENO, ab->p, ab->len);
}

void ab_free(struct abuf *ab) {
    free(ab->p);
}
