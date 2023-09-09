#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "erow.h"
#include "kilo.h"
#include "utils.h"

static void erow_update_rchars(struct erow *erow);

struct erow *erow_create(const char* chars, size_t n_chars, struct buffer *buffer) {
    struct erow *erow = malloc(sizeof(struct erow));

    erow->chars = malloc(n_chars);
    erow->n_chars = n_chars;

    memcpy(erow->chars, chars, erow->n_chars);

    erow->rchars = NULL;
    erow_update_rchars(erow);

    erow->buffer = buffer;

    return erow;
}

void erow_insert_chars(struct erow *erow, const char *chars, size_t n_chars, int at) {
    erow->chars = realloc(erow->chars, erow->n_chars + n_chars);
    memmove(erow->chars + at + n_chars, erow->chars + at, erow->n_chars - at);
    memcpy(erow->chars + at, chars, n_chars);

    erow->n_chars += n_chars;

    erow_update_rchars(erow);

    if (erow->buffer)
        erow->buffer->modified = true;
}

void erow_delete_chars(struct erow *erow, size_t n_chars, int at) {
    memmove(erow->chars + at, erow->chars + at + n_chars, erow->n_chars - at - n_chars);
    erow->chars = realloc(erow->chars, erow->n_chars -= n_chars);

    erow_update_rchars(erow);

    if (erow->buffer)
        erow->buffer->modified = true;
}

int erow_cx_to_rx(struct erow *erow, int cx) {
    if (erow == NULL)
        return 0;

    cx = CLAMP(cx, 0, (int) erow->n_chars);

    int rx = 0;

    for (char *c = erow->chars; c < erow->chars + cx; c++) {
        if (*c == '\t')
            rx += KILO_TAB_STOP - (rx % KILO_TAB_STOP);
        else
            rx++;
    }

    return rx;
}

int erow_rx_to_cx(struct erow *erow, int rx) {
    if (erow == NULL)
        return 0;

    int cx = 0, c_rx = 0;
    while ((size_t) cx < erow->n_chars) {
        if (erow->chars[cx] == '\t')
            c_rx += KILO_TAB_STOP - (c_rx % KILO_TAB_STOP);
        else
            c_rx++;

        cx++;

        if (c_rx >= rx)
            return cx;
    }

    return erow->n_chars;
}

void erow_free(struct erow *erow) {
    if (erow->chars) free(erow->chars);
    if (erow->rchars) free(erow->rchars);
}

static void erow_update_rchars(struct erow *erow) {
    if (erow->rchars)
        free(erow->rchars);

    size_t n_rchars_max = 16;
    erow->rchars = malloc(n_rchars_max);
    erow->n_rchars = 0;

    for (char *c = erow->chars; c < erow->chars + erow->n_chars; c++) {
        if (*c == '\t') {
            int spaces = KILO_TAB_STOP - (erow->n_rchars % KILO_TAB_STOP);

            while (erow->n_rchars + spaces > n_rchars_max)
                erow->rchars = realloc(erow->rchars, n_rchars_max *= 2);

            memset(erow->rchars + erow->n_rchars, ' ', spaces);
            erow->n_rchars += spaces;
        } else {
            if (erow->n_rchars + 1 > n_rchars_max)
                erow->rchars = realloc(erow->rchars, n_rchars_max *= 2);

            erow->rchars[erow->n_rchars++] = *c;
        }
    }

    erow->rchars = realloc(erow->rchars, erow->n_rchars);
}
