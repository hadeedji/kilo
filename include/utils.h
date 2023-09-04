#ifndef UTIL_H
#define UTIL_H

#define CTRL_KEY(key) ((key) & 0x1f)

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

typedef int ERRCODE;
#define RETURN(code) do {errcode = code; goto END;} while(0)

void die(const char *context);

/*****************************************************************************/

struct append_buf {
    char *chars;
    int n_chars;
};

struct append_buf *ab_create(void);
void ab_append(struct append_buf *ab, const char *chars, int n_chars);
void ab_free(struct append_buf *ab);

#endif // APPEND_BUF_H
