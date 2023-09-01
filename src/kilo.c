#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "buffer.h"
#include "kilo.h"
#include "ui.h"
#include "input.h"
#include "terminal.h"

void editor_init(char *filename);
void editor_set_message(const char *, ...);

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

    E.current_buf = buffer_create();
    if (filename)
        buffer_read_file(E.current_buf, filename);

    editor_set_message("Welcome to kilo. Press CTRL-Q to quit.");
}

void editor_set_message(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E.message, sizeof(E.message), fmt, ap);
  va_end(ap);

  E.message_time = time(NULL);
}
