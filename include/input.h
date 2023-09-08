#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

#define CTRL_KEY(key) ((key) & 0x1f)

enum KEYS {
    TAB = 9,
    ENTER = 13,
    ESCAPE = 27,
    BACKSPACE = 127,
    HOME = 0x100,
    DEL,
    PG_UP,
    PG_DOWN,
    END,
    ARROW_UP,
    ARROW_DOWN,
    ARROW_LEFT,
    ARROW_RIGHT,
    NOP
};
typedef uint16_t KEY;

void input_process_key(KEY c);

#endif // INPUT_H
