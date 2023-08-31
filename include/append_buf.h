#ifndef APPEND_BUF_H
#define APPEND_BUF_H

// A simple append only buffer to build strings

struct append_buf {
    char *chars;
    int n_chars;
};

struct append_buf *ab_init(void);
void ab_append(struct append_buf *ab, const char *chars, int n_chars);
void ab_free(struct append_buf *ab);

#endif // APPEND_BUF_H
