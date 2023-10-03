#ifndef KILO_H
#define KILO_H

#define KILO_TAB_STOP 4

#include <termios.h>
#include <time.h>

struct editor_state {
    int screenrows, screencols;
    int quit_times;

    struct buffer *current_buf;
    struct termios orig_termios;

    char message[256];
    time_t message_time;
};
extern struct editor_state E;

void editor_set_message(const char *fmt, ...);
char *editor_prompt(const char *prompt);

extern char *error_message;
void error_set_message(const char *prefix);

#endif // KILO_H
