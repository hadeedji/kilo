CCFLAGS := -Wall -Wextra -Wpedantic -Werror -std=c99

kilo: kilo.c
	$(CC) $(CCFLAGS) kilo.c -o kilo
