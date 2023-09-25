SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:src/%.c=build/%.o)
DEPS := $(OBJS:.o=.d)

CFLAGS := -Wall -Wextra -Iinclude -DKILO_COMMIT_HASH=$(shell git rev-parse --short HEAD) -MMD -MP -std=c99 -ggdb

kilo: $(OBJS)
	gcc $^ -o $@

$(OBJS): build/%.o: src/%.c
	@mkdir -p build
	gcc $(CFLAGS) -c $< -o $@

clean:
	rm -rf build kilo

.PHONY: clean
-include $(DEPS)
