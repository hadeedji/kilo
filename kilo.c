#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

void enable_raw_mode();
void disable_raw_mode();
int get_window_size(int *, int *);

void editor_init();
void editor_redraw_screen();
void editor_clear_screen();
void editor_draw_rows();
void editor_process_key();

void die(const char *);

#define CTRL_KEY(key) ((key) & 0x1f)

struct {
    struct termios orig_termios;
    int rows;
    int cols;
} E;

int main() {
    editor_init();
    enable_raw_mode();

    while (1) {
        editor_redraw_screen();
        editor_process_key();
    }

    return 0;
}

void enable_raw_mode() {
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disable_raw_mode);

    struct termios raw = E.orig_termios;
    cfmakeraw(&raw);

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

void disable_raw_mode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}

int get_window_size(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != -1 && ws.ws_col != 0) {
        *rows = ws.ws_row;
        *cols = ws.ws_col;

        return 0;
    }

    return -1;
}

void editor_init() {
    if (!isatty(STDIN_FILENO)) {
        printf("kilo only supports a terminal at standard in. Exiting.");
        exit(1);
    }

    if (get_window_size(&E.rows, &E.cols) == -1) die("get_window_size");
}

void editor_redraw_screen() {
    editor_clear_screen();

    editor_draw_rows();

    write(STDIN_FILENO, "\x1b[H", 3);
}

void editor_clear_screen() {
    write(STDIN_FILENO, "\x1b[2J", 4);
    write(STDIN_FILENO, "\x1b[H", 3);
}

void editor_draw_rows() {
    for (int y = 0; y < E.rows - 1; y++)
        write(STDIN_FILENO, "~\r\n", 3);
    write(STDIN_FILENO, "~", 1);
}

void editor_process_key() {
    char c;
    read(STDIN_FILENO, &c, 1);

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

void die(const char *s) {
    editor_clear_screen();

    perror(s);
    exit(1);
}
