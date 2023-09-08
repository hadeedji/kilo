#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "buffer.h"
#include "input.h"
#include "kilo.h"
#include "terminal.h"
#include "ui.h"
#include "utils.h"

void editor_init(char *filename);

struct editor_state E;

int main(int argc, char **argv) {
    editor_init(argc >= 2 ? argv[1] : NULL);

    while (true) {
        ui_draw_screen();
        input_process_key();
    }

    return 0;
}

void editor_init(char *filename) {
    if (!isatty(STDIN_FILENO)) {
        printf("kilo only supports a terminal at standard in. Exiting.");
        exit(1);
    }

    if (terminal_enable_raw() == -1) die("term_enable_raw");

    E.cx = E.cy = E.rx = 0;
    E.row_off = E.col_off = 0;

    if (terminal_get_win_size(&E.screenrows, &E.screencols) == -1)
        die("term_get_win_size");
    E.screenrows -= 2;
    E.quit_times = 3;

    E.current_buf = buffer_create();
    if (filename)
        buffer_read_file(E.current_buf, filename);

    editor_set_message("Welcome to kilo! | CTRL-Q: Quit | CTRL-S: SAVE");
    terminal_clear();
}

void editor_set_message(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E.message, sizeof(E.message), fmt, ap);
  va_end(ap);

  E.message_time = time(NULL);
}

// TODO: Implement a fully featured line editor
char *editor_prompt(const char *prompt) {
    size_t buffer_size = 64;
    char *buffer = malloc(buffer_size);

    size_t buffer_len = 0;
    buffer[buffer_len] = '\0';

    while (true) {
        editor_set_message(prompt, buffer);
        ui_draw_screen();

        KEY key = terminal_read_key();
        if (key == ESCAPE) {
            editor_set_message("");
            free(buffer);
            return NULL;
        } else if (key == ENTER) {
            if (buffer_len > 0) {
                editor_set_message("");
                buffer = realloc(buffer, buffer_len + 1);
                return buffer;
            }
        } else if (isprint(key)) {
            if (buffer_len >= buffer_size - 1) {
                buffer_size *= 2;
                buffer = realloc(buffer, buffer_size);
            }

            buffer[buffer_len++] = key;
            buffer[buffer_len] = '\0';
        }
    }

    return buffer;
}
