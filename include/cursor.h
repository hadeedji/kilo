#ifndef CURSOR_H
#define CURSOR_H

#include "buffer.h"

struct buffer;

void cursor_move(struct buffer *buffer, int dx, int dy);

#endif // CURSOR_H
