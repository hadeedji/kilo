CFLAGS := -ggdb -Wall -Wextra -Werror -Wstrict-prototypes -std=c99

kilo: kilo.c
	$(CC) $(CFLAGS) kilo.c -o kilo

clean:
	rm -f ./kilo
