#ifndef UTIL_H
#define UTIL_H

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))
#define CLAMP(value, min, max) MIN(MAX(value, min), max)

#define _STRINGIZE(x) #x
#define STRINGIZE(x) _STRINGIZE(x)

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
