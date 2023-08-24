#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

void enable_raw_mode();
void disable_raw_mode();

int get_window_size(int *, int *);
int read_cursor_position(int *, int *);

void editor_init();
void editor_redraw_screen();
void editor_clear_screen();
void editor_draw_rows();
void editor_process_key();
char editor_read_key();

void die(const char *);

#define CTRL_KEY(key) ((key) & 0x1f)

struct {
    struct termios orig_termios;
    int rows;
    int cols;
    bool raw;
} E = { .raw = false };

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
    if (E.raw) return;

    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disable_raw_mode);

    struct termios raw = E.orig_termios;
    cfmakeraw(&raw);

    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");

    E.raw = true;
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
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    if (read(STDIN_FILENO, buf, 1) != 1 && *buf != '\x1b') return -1;
    if (read(STDIN_FILENO, buf, 1) != 1 && *buf != '[') return -1;

    do {
        if (i == sizeof(buf)) return -1;
        if (read(STDIN_FILENO, buf+i, 1) == 0) break;
    } while(buf[i++] != 'R');
    buf[i-1] = '\0';

    if (sscanf(buf, "%d;%d", rows, cols) != 2) return -1;
    
    return 0;
}

void editor_redraw_screen() {
    editor_clear_screen();

    editor_draw_rows();

    write(STDOUT_FILENO, "\x1b[H", 3);
}

void editor_clear_screen() {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
}

void editor_draw_rows() {
    for (int y = 0; y < E.rows - 1; y++)
        write(STDOUT_FILENO, "~\r\n", 3);
    write(STDOUT_FILENO, "~", 1);
}

void editor_process_key() {
    char c = editor_read_key();
    switch (c) {
        case CTRL_KEY('Q'):
            editor_clear_screen();
            exit(0);
            break;
        default:
            printf("%d\r\n", c);
            break;
    }
}

char editor_read_key() {
    char c;
    while (read(STDIN_FILENO, &c, 1) == 0);
    return c;
}

void die(const char *s) {
    editor_clear_screen();

    perror(s);
    exit(1);
}
