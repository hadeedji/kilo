#ifndef BUFFER_H
#define BUFFER_H

#include <stdbool.h>
#include <stddef.h>

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
    bool modified;
};

struct buffer *buffer_create(void);
void buffer_read_file(struct buffer *buffer, const char *filename);
void buffer_insert_row(struct buffer *buffer, const char *chars, int n_chars, int at);
void buffer_delete_row(struct buffer *buffer, int at);
ERRCODE buffer_write_file(struct buffer *buffer, int *bytes_written);

void erow_update_rendering(struct erow *erow);
void erow_append_string(struct erow *erow, const char *s, size_t s_len);
void erow_insert_char(struct erow *erow, int at, char c);
void erow_delete_char(struct erow *erow, int at);
int erow_cx_to_rx(struct erow *erow, int cx);
int erow_rx_to_cx(struct erow *erow, int rx);
void erow_free(struct erow *erow);

#endif // BUFFER_H
