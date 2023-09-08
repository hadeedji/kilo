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

// TODO: Implement a fully featured line editor here
char *editor_prompt(const char *prompt) {
    size_t buf_cap = 64;
    size_t buf_size = 0;
    char *buf = malloc(buf_cap);

    buf[buf_size] = '\0';

    while (true) {
        editor_set_message(prompt, buf);
        ui_draw_screen();

        KEY key = terminal_read_key();
        if (key == ESCAPE) goto failure;
        else if (key == ENTER) {
            if (buf_size > 0) goto success;
            else goto failure;
        } else if (isprint(key)) {
            if (buf_size + 1 >= buf_cap)
                buf = realloc(buf, buf_cap *= 2);

            buf[buf_size] = key;
            buf_size++;
            buf[buf_size] = '\0';
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
