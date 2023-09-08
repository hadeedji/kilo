#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "erow.h"
#include "utils.h"

static void buffer_free_rows(struct buffer *buffer);
static char *buffer_get_string(struct buffer *buffer, size_t *len_p);

struct buffer *buffer_create(void) {
    struct buffer *buffer = malloc(sizeof(struct buffer));

    buffer->filename = NULL;

    buffer->cx = buffer->cy = buffer->rx = 0;
    buffer->row_off = buffer->col_off = 0;

    buffer->rows = NULL;
    buffer->n_rows = 0;

    buffer->modified = false;

    return buffer;
}

ERRCODE buffer_read_file(struct buffer *buffer, const char *filename) {
    ERRCODE errcode = 0;

    if (buffer->filename) free(buffer->filename);
    size_t filename_len = strlen(filename);
    buffer->filename = malloc(filename_len);
    memcpy(buffer->filename, filename, filename_len);

    if (buffer->rows)
        buffer_free_rows(buffer);

    size_t buf_cap = 64;
    char *buf = malloc(buf_cap);

    FILE *file = fopen(filename, "r");
    if (file == NULL)
        RETURN(-1);

    while (true) {
        char c;
        size_t buf_size = 0;

        while ((c = getc(file)) != '\n' && c != EOF) {
            if (buf_size >= buf_cap)
                buf = realloc(buf, buf_cap *= 2);

            buf[buf_size++] = c;
        }

        struct erow* erow = erow_create(buf, buf_size, buffer);
        buffer_insert_row(buffer, erow, buffer->n_rows);

        if (c == EOF)
            break;
    }

END:
    if (file != NULL)
        fclose(file);

    if (buf != NULL)
        free(buf);

    buffer->modified = false;

    return errcode;
}

ERRCODE buffer_write_file(struct buffer *buffer, size_t *bytes_written) {
    ERRCODE errcode = 0;

    FILE *file = NULL;
    char *write_buffer = NULL;

    if (buffer->filename == NULL)
        RETURN(-1);

    size_t n_chars;
    write_buffer = buffer_get_string(buffer, &n_chars);

    file = fopen(buffer->filename, "w");
    if (file == NULL)
        RETURN(-2);

    *bytes_written = fwrite(write_buffer, 1, n_chars, file);
    if (*bytes_written != n_chars)
        RETURN(-3);

END:
    if (file != NULL)
        fclose(file);

    if (write_buffer)
        free(write_buffer);

    if (errcode == 0)
        buffer->modified = false;

    return errcode;
}

ERRCODE buffer_get_row_chars(struct buffer *buffer, char **chars, size_t *n_chars, int at) {
    if (buffer == NULL)
        return -1;

    if (!(0 <= at && at <= buffer->n_rows))
        return -2;

    if (chars) *chars = buffer->rows[at]->chars;
    if (n_chars) *n_chars = buffer->rows[at]->n_chars;

    return 0;
}

ERRCODE buffer_get_row_rchars(struct buffer *buffer, char **rchars, size_t *n_rchars, int at) {
    if (buffer == NULL)
        return -1;

    if (!(0 <= at && at <= buffer->n_rows))
        return -2;

    if (rchars) *rchars = buffer->rows[at]->rchars;
    if (n_rchars) *n_rchars = buffer->rows[at]->n_rchars;

    return 0;
}

void buffer_insert_row(struct buffer *buffer, struct erow *erow, int at) {
    if (!(0 <= at && at <= buffer->n_rows))
        return;

    buffer->rows = realloc(buffer->rows, sizeof(struct erow *) * (buffer->n_rows + 1));
    memmove(buffer->rows + at + 1, buffer->rows + at, sizeof(struct erow *) * (buffer->n_rows - at));

    buffer->rows[at] = erow;
    buffer->n_rows++;
}

void buffer_delete_row(struct buffer *buffer, int at) {
    if (!(0 <= at && at < buffer->n_rows))
        return;

    erow_free(buffer->rows[at]);
    memmove(buffer->rows + at, buffer->rows + at + 1, sizeof(struct erow *) * (buffer->n_rows - at - 1));
    buffer->n_rows--;

    buffer->modified = true;
}

struct erow *buffer_get_crow(struct buffer *buffer) {
    return (buffer->cy < buffer->n_rows ? buffer->rows[buffer->cy]: NULL);
}

void buffer_free(struct buffer *buffer) {
    free(buffer->filename);

    buffer_free_rows(buffer);
}

size_t buffer_get_crow_len(struct buffer *buffer) {
    struct erow *crow = buffer_get_crow(buffer);

    return crow ? crow->n_chars : 0;
}

static void buffer_free_rows(struct buffer *buffer) {
    for (int i = 0; i < buffer->n_rows; i++)
        erow_free(buffer->rows[i]);

    free(buffer->rows);

    buffer->rows = NULL;
    buffer->n_rows = 0;
}

static char *buffer_get_string(struct buffer *buffer, size_t *n_chars) {
    *n_chars = 0;

    for (int i = 0; i < buffer->n_rows; i++)
        *n_chars += buffer->rows[i]->n_chars + 1;

    char *write_buffer = malloc(*n_chars);
    for (int i = 0, j = 0; i < buffer->n_rows; j++, i++) {
        struct erow *row = buffer->rows[i];

        memcpy(write_buffer + j, row->chars, row->n_chars);
        j += row->n_chars;
        write_buffer[j++] = '\n';
    }

    return write_buffer;
}
