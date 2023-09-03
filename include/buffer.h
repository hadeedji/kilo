#ifndef BUFFER_H
#define BUFFER_H

#include "utils.h"

struct erow {
    char *chars;
    int n_chars;

    char *rchars;
    int n_rchars;
};

struct buffer {
    char *filename;
    struct erow *rows;
    int n_rows;
};

struct buffer *buffer_create(void);
void buffer_read_file(struct buffer *buffer, const char *filename);
void buffer_append_row(struct buffer *buffer, const char *chars, int n_chars);
ERRCODE buffer_write_file(struct buffer *buffer);

void erow_update_rendering(struct erow *erow);
void erow_insert_char(struct erow *erow, int at, char c);
int erow_cx_to_rx(struct erow *erow, int cx);
int erow_rx_to_cx(struct erow *erow, int rx);

#endif // BUFFER_H
