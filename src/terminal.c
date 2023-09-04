#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "kilo.h"
#include "utils.h"
#include "terminal.h"

ERRCODE terminal_enable_raw(void) {
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
        return -1;

    struct termios raw = E.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 1;

    atexit(terminal_disable_raw);
    return tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void terminal_disable_raw(void) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die ("term_disable_raw");
}

ERRCODE terminal_clear(void) {
    if (write(STDOUT_FILENO, "\x1b[2J", 4) != 4) return -1;
    if (write(STDOUT_FILENO, "\x1b[H", 3) != 3) return -1;
    return 0;

}

ERRCODE terminal_cursor_visibility(enum cursor_visibility visibility) {
    if (write(STDOUT_FILENO, (visibility ? "\x1b[?25l" : "\x1b[?25h"), 6) != 6)
        return -1;

    return 0;
}

ERRCODE terminal_get_win_size(int *row, int *col) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1 && ws.ws_col != 0) {
        *row = ws.ws_row;
        *col = ws.ws_col;

        return 0;
    } else {
        // Fallback implementation
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;

        return terminal_get_cursor_pos(row, col);
    }
}

ERRCODE terminal_get_cursor_pos(int *row, int *col) {
    char buf[16];

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    for (int i = 0; i < (int) sizeof(buf); i++) {
        if (read(STDIN_FILENO, buf+i, 1) == 0 || buf[i] == 'R') {
            buf[i] = '\0';
            break;
        }
    }

    if (sscanf(buf, "\x1b[%d;%d", row, col) == EOF)
        return -1;

    return 0;
}

ERRCODE terminal_set_cursor_pos(int row, int col) {
    char buf[16];

    int len = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", row, col);
    len = (len >= (int) sizeof(buf) ? (int) sizeof(buf) - 1 : len);

    if ((int) write(STDOUT_FILENO, buf, len) != len) return -1;
    return 0;
}

KEY terminal_read_key(void) {
    char c;
    int n;
    while (read(STDIN_FILENO, &c, 1) == 0);

    if (c == '\x1b') {
        char buf[8] = { '\0' };

        for (int i = 0; i < (int) sizeof(buf); i++) {
            if (read(STDIN_FILENO, buf+i, 1) == 0) {
                if (i == 0) return ESCAPE;
                else break;
            }

            char escape_char;
            int escape_int;

            n = 0;
            sscanf(buf, "[%c%n", &escape_char, &n);
            if (n > 0) {
                switch (escape_char) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME;
                    case 'F': return END;
                }
            }

            n = 0;
            sscanf(buf, "O%c%n", &escape_char, &n);
            if (n > 0) {
                switch (escape_char) {
                    case 'H': return HOME;
                    case 'F': return END;
                }
            }

            n = 0;
            sscanf(buf, "[%d~%n", &escape_int, &n);
            if (n > 0) {
                switch (escape_int) {
                    case 1:
                    case 7:
                        return HOME;
                    case 3:
                        return DEL;
                    case 4:
                    case 8:
                        return END;
                    case 5:
                        return PG_UP;
                    case 6:
                        return PG_DOWN;
                }
            }
        }

        return NOP;
    }

    return (KEY) c;
}

