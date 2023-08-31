#include <stdlib.h>
#include <string.h>

#include "append_buf.h"

struct append_buf *ab_init(void) {
    size_t buf_size = sizeof(struct append_buf);
    struct append_buf *sb = (struct append_buf *) malloc(buf_size);

    sb->chars = NULL;
    sb->n_chars = 0;

    return sb;
}

void ab_append(struct append_buf *sb, const char *chars, int n_chars) {
    sb->chars = realloc(sb->chars, sb->n_chars + n_chars);

    memcpy(sb->chars + sb->n_chars, chars, n_chars);
    sb->n_chars += n_chars;
}

void ab_free(struct append_buf *sb) {
    free(sb->chars);
    free(sb);
}
