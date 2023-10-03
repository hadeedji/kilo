#define _POSIX_C_SOURCE 199309L

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "buffer.h"
#include "input.h"
#include "kilo.h"
#include "terminal.h"
#include "ui.h"
#include "utils.h"

void editor_init(char *filename);
void editor_resize();

struct editor_state E;

int main(int argc, char **argv) {
    editor_init(argc >= 2 ? argv[1] : NULL);

    while (true) {
        ui_draw_screen();
        input_process_key(terminal_read_key());
    }

    return 0;
}

void editor_init(char *filename) {
    if (!isatty(STDIN_FILENO)) {
        printf("kilo only supports a terminal at standard in. Exiting.");
        exit(1);
    }

    if (terminal_enable_raw() == -1) die("term_enable_raw");

    if (terminal_get_win_size(&E.screenrows, &E.screencols) == -1)
        die("term_get_win_size");
    E.quit_times = 3;

    E.current_buf = buffer_create();
    if (filename)
        buffer_read_file(E.current_buf, filename);

    editor_set_message("Welcome to kilo! | CTRL-Q: Quit | CTRL-S: SAVE");
    terminal_clear();
    error_message = NULL;

    struct sigaction sa;
    sa.sa_handler = editor_resize;
    sigaction(SIGWINCH, &sa, NULL);
}

void editor_set_message(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E.message, sizeof(E.message), fmt, ap);
  va_end(ap);

  E.message_time = time(NULL);
}

// TODO: This could use some work
char *editor_prompt(const char *prompt) {
    size_t buf_cap = 64;
    size_t buf_size = 0;
    char *buf = malloc(buf_cap);

    char *prompt_prefix = strstr(prompt, "%s");
    int prefix_len = (prompt_prefix ? prompt_prefix - prompt : 0);

    buf[buf_size] = '\0';

    while (true) {
        editor_set_message(prompt, buf);
        ui_draw_screen();

        if (prompt_prefix)
            terminal_set_cursor_pos(E.screenrows + 2, prefix_len + buf_size + 1);

        KEY key = terminal_read_key();
        switch (key) {
            case BACKSPACE:
                if (buf_size > 0)
                    buf[--buf_size] = '\0';
                break;
            case ENTER:
                if (buf_size > 0) goto success;
                else goto failure;
                break;
            case ESCAPE:
                goto failure;
                break;
            default:
                if (isprint(key)) {
                    if (buf_size + 1 >= buf_cap)
                        buf = realloc(buf, buf_cap *= 2);

                    buf[buf_size] = key;
                    buf_size++;
                    buf[buf_size] = '\0';
                }
        }
    }

failure:
    free(buf);
    buf = NULL;
success:
    if (buf)
        buf = realloc(buf, buf_size + 1);

    editor_set_message("");
    return buf;
}

void editor_resize() {
    if (terminal_get_win_size(&E.screenrows, &E.screencols) == -1)
        die("term_get_win_size");

    ui_draw_screen();
}

/*****************************************************************************/

char *error_message;

void error_set_message(const char *prefix) {
    if (error_message)
        free(error_message);

    const char *strerror_msg = strerror(errno);
    const char *fmt = "%s: %s\n";
    size_t len = strlen(prefix) + 2 + strlen(strerror_msg) + 2;

    error_message = malloc(len);
    snprintf(error_message, len, fmt, prefix, strerror_msg);
}
