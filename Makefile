SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:src/%.c=build/%.o)
DEPS := $(OBJS:.o=.d)

CC := gcc
CFLAGS := -Wall -Wextra -Iinclude -DKILO_COMMIT_HASH=$(shell git rev-parse --short HEAD) -MMD -MP -std=c99 -ggdb

kilo: $(OBJS)
	$(CC) $^ -o $@

$(OBJS): build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build kilo

.PHONY: clean
-include $(DEPS)
