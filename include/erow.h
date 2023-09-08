#ifndef EROW_H
#define EROW_H

#include <stdlib.h>

struct buffer;

struct erow {
    char *chars;
    size_t n_chars;

    char *rchars;
    size_t n_rchars;

    struct buffer *buffer;
};

struct erow *erow_create(const char* chars, size_t n_chars, struct buffer *buffer);
void erow_insert_chars(struct erow *erow, const char *chars, size_t n_chars, int at);
void erow_delete_chars(struct erow *erow, size_t n_chars, int at);
int erow_cx_to_rx(struct erow *erow, int cx);
int erow_rx_to_cx(struct erow *erow, int rx);
void erow_free(struct erow *erow);

#endif // EROW_H
