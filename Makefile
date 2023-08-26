CFLAGS := -Wall -Wextra -Wpedantic -Wstrict-prototypes -Werror -std=c99

kilo: kilo.c
	$(CC) $(CFLAGS) kilo.c -o kilo

clean:
	rm -f ./kilo
