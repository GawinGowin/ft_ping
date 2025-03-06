NAME := ft_ping
DNAME := $(NAME)_debug
BUILD_DIR = ./build
BASE_PKG_DIR = cmd/ft_ping/

SOURCE =
SOURCE += cmd/ft_ping/ft_ping.c

HEADER =
HEADER += cmd/ft_ping/ft_ping.h

TESTS =
TESTS += $(shell find ./tests -name '*.cpp' -o -name '*.hpp')

CC := cc
CFLAGS := -Wall -Wextra -Werror -MMD -MP -I$(BASE_PKG_DIR) 
LFALGS := 
DFLAGS := -fdiagnostics-color=always -g3 -fsanitize=address

OBJS := $(SOURCE:.c=.o)
DOBJS := $(SOURCE:.c=_d.o)
DEP = $(OBJS:.o=.d)
DDEP = $(DOBJS:.o=.d)

.PHONY: all
all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $^ $(LFALGS) -o $@

.PHONY: debug
debug: $(DNAME)

$(DNAME): $(DOBJS)
	$(CC) $(CFLAGS) $(DFLAGS) $^ $(LFALGS) -o $@

-include $(DEP)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DDEP)
%_d.o: %.c
	$(CC) $(CFLAGS) $(DFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(DOBJS) $(DEP) $(DDEP)

.PHONY: fclean
fclean: clean
	rm -f $(NAME) $(DNAME)
	rm -rf $(BUILD_DIR)

.PHONY: re
re: fclean all

.PHONY: fmt
fmt:
	clang-format -i --style=file $(SOURCE) $(HEADER) $(TESTS)

.PHONY: build
build:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_VERBOSE_MAKEFILE=ON ..  && make

.PHONY: test
test: build
	cd $(BUILD_DIR)/tests && make test

.PHONY: cov
cov: build
	lcov --capture --directory . --output-file coverage.info
	lcov --remove coverage.info '/usr/*' --output-file coverage.info # filter system-files
	lcov --list coverage.info # debug info