#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "kilo.h"
#include "utils.h"

static void buffer_free_rows(struct buffer *buffer);
static char *buffer_get_string(struct buffer *buffer, size_t *len_p);

struct buffer *buffer_create(void) {
    struct buffer *buffer = malloc(sizeof(struct buffer));

    buffer->filename = NULL;
    buffer->rows = NULL;
    buffer->n_rows = 0;
    buffer->modified = false;

    return buffer;
}

// TODO: Handle file not existing
void buffer_read_file(struct buffer *buffer, const char *filename) {
    if (buffer->filename)
        free(buffer->filename);

    if (buffer->rows)
        buffer_free_rows(buffer);

    size_t filename_len = strlen(filename);
    buffer->filename = malloc(filename_len);
    memcpy(buffer->filename, filename, filename_len);

    FILE *file = fopen(filename, "r");
    if (!file) die("fopen");

    size_t line_buffer_size = 64;
    char *line_buffer = malloc(line_buffer_size);

    bool file_remaining = true;
    while (file_remaining) {
        char c;
        int i;

        for (i = 0; (c = getc(file)) != '\n' && c != EOF; i++) {
            if (i >= (int) line_buffer_size)
                line_buffer = realloc(line_buffer, line_buffer_size *= 2);

            line_buffer[i] = c;
        }

        buffer_append_row(buffer, line_buffer, i);
        file_remaining = (c != EOF);
    }

    free(line_buffer);
    fclose(file);

    buffer->modified = false;
}

ERRCODE buffer_write_file(struct buffer *buffer) {
    ERRCODE errcode = 0;

    int fd = -1;
    char *write_buffer = NULL;

    if (buffer->filename == NULL)
        RETURN(-1);

    size_t len;
    write_buffer = buffer_get_string(buffer, &len);

    fd = open(buffer->filename, O_RDWR | O_TRUNC | O_CREAT, 0644);
    if (fd == -1)
        RETURN(-2);

    if (write(fd, write_buffer, len) != (ssize_t) len)
        RETURN(-3);

END:
    if (fd != -1)
        close(fd);

    if (write_buffer)
        free(write_buffer);

    if (errcode == 0)
        buffer->modified = false;

    return errcode;
}

void buffer_append_row(struct buffer *buffer, const char *chars, int n_chars) {
    buffer->rows = realloc(buffer->rows, sizeof(struct erow) * (buffer->n_rows + 1));
    struct erow *new_row = &buffer->rows[buffer->n_rows++];

    new_row->chars = malloc(n_chars);
    memcpy(new_row->chars, chars, n_chars);
    new_row->n_chars = n_chars;

    new_row->rchars = NULL;
    new_row->n_rchars = 0;

    erow_update_rendering(new_row);

    buffer->modified = true;
}

void buffer_free(struct buffer *buffer) {
    free(buffer->filename);
    buffer_free_rows(buffer);
}

static void buffer_free_rows(struct buffer *buffer) {
    for (int i = 0; i < buffer->n_rows; i++) {
        if (buffer->rows->chars) free(buffer->rows->chars);
        if (buffer->rows->rchars) free(buffer->rows->rchars);
    }
}

static char *buffer_get_string(struct buffer *buffer, size_t *len_p) {
    *len_p = 0;

    for (int i = 0; i < buffer->n_rows; i++)
        *len_p += buffer->rows[i].n_chars + 1;

    char *write_buffer = malloc(*len_p);
    char *p = write_buffer;
    for (int i = 0; i < buffer->n_rows; i++) {
        struct erow *row = buffer->rows+i;
        memcpy(p, row->chars, row->n_chars);
        p += row->n_chars;
        *p = '\n';
        p++;
    }

    return write_buffer;
}

/*****************************************************************************/

void erow_update_rendering(struct erow *erow) {
    size_t line_buffer_size = 64;
    char *line_buffer = malloc(line_buffer_size);
    size_t n_rchars = 0;

    for (int i = 0; i < erow->n_chars; i++) {
        switch (erow->chars[i]) {
            case '\t': {
                int spaces = KILO_TAB_STOP - (n_rchars % KILO_TAB_STOP);
                while (spaces--) {
                    if (n_rchars >= line_buffer_size)
                        line_buffer = realloc(line_buffer, 2*line_buffer_size);

                    line_buffer[n_rchars++] = ' ';
                }
                break;
            }
            default:
                if (n_rchars >= line_buffer_size)
                    line_buffer = realloc(line_buffer, 2*line_buffer_size);

                line_buffer[n_rchars++] = erow->chars[i];
        }
    }

    if (erow->rchars)
        free(erow->rchars);

    erow->rchars = malloc(n_rchars);
    erow->n_rchars = n_rchars;
    memcpy(erow->rchars, line_buffer, n_rchars);

    free(line_buffer);
}

void erow_insert_char(struct erow *erow, int at, char c) {
    if (at < 0) at = 0;
    if (at > erow->n_chars) at = erow->n_chars;

    erow->chars = realloc(erow->chars, erow->n_chars + 1);
    memmove(erow->chars + at + 1, erow->chars + at, erow->n_chars - at);

    erow->chars[at] = c;
    erow->n_chars++;

    erow_update_rendering(erow);

    // TODO: Seems like a bad idea
    E.current_buf->modified = true;
}

void erow_delete_char(struct erow *erow, int at) {
    if (at < 0) at = 0;
    if (at > erow->n_chars) at = erow->n_chars;

    if (at != 0) {
        memmove(erow->chars + at, erow->chars + at + 1, erow->n_chars - at);
        erow->n_chars--;
        erow->chars = realloc(erow->chars, erow->n_chars);
        E.cx--;
        erow_update_rendering(erow);
    }

    // TODO
    E.current_buf->modified = true;
}

int erow_cx_to_rx(struct erow *erow, int cx) {
    int rx = 0;

    if (erow == NULL)
        return 0;

    for (int i = 0; i < MIN(cx, erow->n_chars); i++) {
        if (erow->chars[i] == '\t')
            rx += KILO_TAB_STOP - (rx % KILO_TAB_STOP);
        else rx++;
    }

    return rx;
}

int erow_rx_to_cx(struct erow *erow, int rx) {
    int cx = 0, _rx = 0;

    if (erow == NULL)
        return 0;

    while (_rx < rx && cx < erow->n_chars) {
        if (erow->chars[cx] == '\t') {
            _rx += KILO_TAB_STOP - (_rx % KILO_TAB_STOP);
        } else _rx++;

        cx++;
    }

    return (_rx >= rx ? cx : erow->n_chars);
}
