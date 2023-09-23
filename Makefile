HASH := $(shell git rev-parse --short HEAD)
SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:src/%.c=build/%.o)
DEPS := $(OBJS:%.o=%.d)

WARNING_FLAGS := -Wall -Wextra
INCLUDE_FLAGS := -I include

CC := clang
CFLAGS := $(WARNING_FLAGS) $(INCLUDE_FLAGS) -DKILO_COMMIT_HASH=$(HASH) -MMD -MP -std=c99 -g

kilo: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJS): build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: install clean
install: kilo
	cp kilo ~/bin

clean:
	rm -rf build
	rm -f ./kilo

-include $(DEPS)
