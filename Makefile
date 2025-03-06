NAME := ft_ping
DNAME := $(NAME)_debug
BUILD_DIR = ./build
BASE_PKG_DIR = cmd/ft_ping/

SOURCE =
SOURCE += cmd/ft_ping/ft_ping.c
SOURCE += cmd/ft_ping/usecases.c
SOURCE += cmd/ft_ping/utils.c
SOURCE += cmd/ft_ping/setup.c

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

COV_INFO = coverage.info
TEST_LOG = build/tests/Testing/Temporary/LastTest.log

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
	rm -f $(OBJS) $(DOBJS) $(DEP) $(DDEP) $(COV_INFO)

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

$(TEST_LOG): test


$(COV_INFO): $(TEST_LOG)
	lcov --capture --directory . --output-file $(COV_INFO)
	lcov --remove $(COV_INFO) '/usr/*' --output-file $(COV_INFO) # filter system-files

.PHONY: cov
cov: $(COV_INFO)
	lcov --list $(COV_INFO) # debug info