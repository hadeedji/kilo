#ifndef BUFFER_H
#define BUFFER_H

#include <stdbool.h>
#include <stddef.h>

#include "erow.h"
#include "utils.h"

struct erow;

struct buffer {
    char *filename;

    int cx, cy, rx;
    int row_off, col_off;

    struct erow **rows;
    int n_rows;

    bool modified;
};

struct buffer *buffer_create(void);
ERRCODE buffer_read_file(struct buffer *buffer, const char *filename);
ERRCODE buffer_write_file(struct buffer *buffer, size_t *bytes_written);
void buffer_insert_row(struct buffer *buffer, struct erow *erow, int at);
void buffer_delete_row(struct buffer *buffer, int at);
struct erow *buffer_get_crow(struct buffer *buffer);
size_t buffer_get_crow_len(struct buffer *buffer);
void buffer_free(struct buffer *buffer);


#endif // BUFFER_H
