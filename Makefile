CCFLAGS := -Wall -Wextra -Wpedantic -Werror -std=c99 -D_DEFAULT_SOURCE

kilo: kilo.c
	$(CC) $(CCFLAGS) kilo.c -o kilo
