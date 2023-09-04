#ifndef COMMANDS_H
#define COMMANDS_H

#include "input.h"

void command_quit(void);
void command_move_cursor(KEY key);
void command_insert_char(char c);
void command_delete_char(void);
void command_save_buffer(void);

#endif // COMMANDS_H
