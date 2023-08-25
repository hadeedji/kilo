#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define CTRL_KEY(key) ((key) & 0x1f)

/*****************************************************************************/

struct screen_buf;
struct input_buf;

typedef uint16_t KEY;

struct {
    struct termios orig_termios;
    struct input_buf *ib;
    int screenrows, screencols;
    int cx, cy;
    bool running;
} E;

enum keys {
    HOME = 0x100 + 1,
    DEL,
    PG_UP,
    PG_DOWN,
    END,
    ARROW_UP,
    ARROW_DOWN,
    ARROW_LEFT,
    ARROW_RIGHT,
    NOP
};

/*****************************************************************************/

void editor_init();
void editor_redraw_screen();
void editor_draw_rows(struct screen_buf *);
void editor_process_key();
void editor_free();

struct screen_buf *sb_init();
void sb_append(struct screen_buf *, const char *);
void sb_write(struct screen_buf *);
void sb_free(struct screen_buf *);

struct input_buf *ib_init(size_t);
KEY ib_read(struct input_buf *);
void ib_write(struct input_buf *, KEY);
bool ib_empty(struct input_buf *);
void ib_free(struct input_buf *);

int term_enable_raw();
void term_disable_raw();
int term_clear();
int term_cursor_hidden(bool);
int term_get_win_size(int *, int *);
int term_get_cursor_pos(int *, int *);
int term_set_cursor_pos(int, int);
KEY term_read_key();


void die(const char *);

/*****************************************************************************/

int main() {
    editor_init();

    while (E.running) {
        editor_redraw_screen();
        editor_process_key();
    }

    editor_free();
    return 0;
}

/*****************************************************************************/

void editor_init() {
    if (!isatty(STDIN_FILENO)) {
        printf("kilo only supports a terminal at standard in. Exiting.");
        exit(1);
    }

    if (term_enable_raw() == -1) die("term_enable_raw");

    if (term_get_win_size(&E.screenrows, &E.screencols) == -1) die("term_get_win_size");

    E.cx = E.cy = 0;
    E.ib = ib_init(128);
    E.running = true;
}

void editor_redraw_screen() {
    if (term_cursor_hidden(true) == -1) die("term_cursor_hidden");

    struct screen_buf *sb = sb_init();
    editor_draw_rows(sb);
    sb_write(sb);
    sb_free(sb);

    if (term_set_cursor_pos(E.cy + 1, E.cx + 1) == -1) die("term_set_cursor_pos");
    if (term_cursor_hidden(false) == -1) die("term_cursor_hidden");
}

void editor_draw_rows(struct screen_buf *sb) {
    term_set_cursor_pos(1, 1);

    for (int y = 0; y < E.screenrows; y++) {
        sb_append(sb, "~");

        sb_append(sb, "\x1b[K"); // Clear rest of the line
        if (y < E.screenrows - 1) sb_append(sb, "\r\n");
    }
}


void editor_process_key() {
    KEY c = ib_read(E.ib);

    switch (c) {
        case ARROW_LEFT:
            if (E.cx > 0) E.cx--;
            break;
        case ARROW_DOWN:
            if (E.cy < E.screenrows - 1) E.cy++;
            break;
        case ARROW_UP:
            if (E.cy > 0) E.cy--;
            break;
        case ARROW_RIGHT:
            if (E.cx < E.screencols - 1) E.cx++;
            break;

        case HOME:
            E.cx = 0;
            break;
        case END:
            E.cx = E.screencols - 1;
            break;

        case PG_UP:
        case PG_DOWN:
            for (int i = 0; i < E.screenrows; i++)
                 ib_write(E.ib, (c == PG_UP ? ARROW_UP : ARROW_DOWN));
            break;

        case CTRL_KEY('Q'):
            E.running = false;
            break;
    }
}

void editor_free() {
    term_clear();
    ib_free(E.ib);
}

/*****************************************************************************/

struct screen_buf {
    char *s;
    size_t size;
};

struct screen_buf *sb_init() {
    struct screen_buf *sb = (struct screen_buf *) malloc(sizeof(struct screen_buf));

    sb->s = NULL;
    sb->size = 0;

    return sb;
}

void sb_append(struct screen_buf *sb, const char *s) {
    int size = strlen(s);

    sb->s = realloc(sb->s, sb->size + size);
    memcpy(sb->s + sb->size, s, size);

    sb->size += size;
}

void sb_write(struct screen_buf *sb) {
    write(STDOUT_FILENO, sb->s, sb->size);
}

void sb_free(struct screen_buf *sb) {
    free(sb->s);
    free(sb);
}


struct input_buf {
    KEY *carr; // Circular array
    int read_i;
    int write_i;
    size_t capacity; // Actual capacity is one less
};

struct input_buf *ib_init(size_t capacity) {
    struct input_buf *ib = (struct input_buf *) malloc(sizeof(struct input_buf));

    ib->carr = (KEY *) malloc(sizeof(KEY) * capacity);
    ib->write_i = 0;
    ib->read_i  = 0;
    ib->capacity = capacity;

    return ib;
}

void ib_write(struct input_buf *ib, KEY key) {
    if ((ib->write_i + 1) % (int) ib->capacity == ib->read_i) // Buffer full
        die("ib_write");

    ib->carr[ib->write_i] = key;
    ib->write_i = (ib->write_i + 1) % ib->capacity;
}

KEY ib_read(struct input_buf *ib) {
    if (ib_empty(ib)) ib_write(ib, term_read_key());

    KEY key = ib->carr[ib->read_i];
    ib->read_i = (ib->read_i + 1) % ib->capacity;

    return key;
}

bool ib_empty(struct input_buf *ib) {
    return ib->write_i == ib->read_i;
}

void ib_free(struct input_buf *ib) {
    free(ib->carr);
    free(ib);
}

/*****************************************************************************/

int term_enable_raw() {
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) return -1;
    atexit(term_disable_raw);

    struct termios raw = E.orig_termios;
    cfmakeraw(&raw);

    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 10;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return -1;

    return 0;
}

void term_disable_raw() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die ("term_disable_raw");
}

int term_clear() {
    if (write(STDOUT_FILENO, "\x1b[2J", 4) != 4) return -1;
    if (write(STDOUT_FILENO, "\x1b[H", 3) != 3) return -1;
    return 0;

}

int term_cursor_hidden(bool hidden) {
    if (write(STDOUT_FILENO, (hidden ? "\x1b[?25l" : "\x1b[?25h"), 6) != 6)
        return -1;

    return 0;
}

int term_get_win_size(int *row, int *col) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1 && ws.ws_col != 0) {
        *row = ws.ws_row;
        *col = ws.ws_col;

        return 0;
    } else {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;

        return term_get_cursor_pos(row, col);
    }
}

int term_get_cursor_pos(int *row, int *col) {
    char buf[32];

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    for (size_t i = 0; i < sizeof(buf); i++) {
        if (read(STDIN_FILENO, buf+i, 1) == 0 || buf[i] == 'R') {
            buf[i] = '\0';
            break;
        }
    }

    if (sscanf(buf, "\x1b[%d;%d", row, col) == EOF)
        return -1;

    return 0;
}

int term_set_cursor_pos(int row, int col) {
    char buf[16];

    size_t len = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", row, col);
    len = (len >= sizeof(buf) ? sizeof(buf) - 1 : len);

    if (write(STDOUT_FILENO, buf, len) != (ssize_t) len) return -1;
    return 0;
}

KEY term_read_key() {
    char c;
    while (read(STDIN_FILENO, &c, 1) == 0);

    if (c == '\x1b') {
        char buf[8] = { '\0' };

        // Inefficient but I like the way it looks
        for (size_t i = 0; i < sizeof(buf); i++) {
            if (read(STDIN_FILENO, buf+i, 1) == 0) break;

            char escape_char;
            if (sscanf(buf, "[%c~", &escape_char) != EOF) {
                switch (escape_char) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME;
                    case 'F': return END;
                }
            }

            int escape_int;
            if (sscanf(buf, "[%d~", &escape_int) != EOF) {
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

/*****************************************************************************/

void die(const char *s) {
    term_clear();

    perror(s);
    exit(1);
}

