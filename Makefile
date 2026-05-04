NAME := ft_ping
DNAME := $(NAME)_debug
BUILD_DIR := ./build

# ── ソースファイル ──
LIB_SRC := \
	lib/ftping.c \
	lib/ping_config.c \
	lib/ping_icmp.c \
	lib/ping_loop.c \
	lib/ping_schedule.c \
	lib/ping_stats.c \
	lib/shared/shared_error.c \
	lib/shared/shared_net.c \
	lib/shared/shared_parse.c \
	lib/vsock/vsock.c \
	lib/vsock/vsock_dgram.c \
	lib/vsock/vsock_raw.c

SRC_SRC := \
	src/tool_cleanup.c \
	src/tool_getparam.c \
	src/tool_main.c \
	src/tool_output.c \
	src/tool_signal.c

SOURCE := $(LIB_SRC) $(SRC_SRC)

HEADER := $(shell find lib src include -name '*.h')
TESTS  := $(shell find tests -name '*.cpp' -o -name '*.hpp')

# ── コンパイラ設定 ──
CC := cc
INCLUDES := -Iinclude -Iinclude/ft_ping -Ilib -Isrc
CFLAGS := -Wall -Wextra -Werror -MMD -MP $(INCLUDES)
LFLAGS :=
DFLAGS := -fdiagnostics-color=always -g3 -fsanitize=address

OBJS := $(SOURCE:.c=.o)
DOBJS := $(SOURCE:.c=_d.o)
DEP := $(OBJS:.o=.d)
DDEP := $(DOBJS:.o=.d)

COV_INFO := coverage.info
TEST_LOG := build/tests/Testing/Temporary/LastTest.log

# ── 直接コンパイル経路（CMake 不使用） ──
.PHONY: all
all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $^ $(LFLAGS) -o $@

.PHONY: debug
debug: $(DNAME)

$(DNAME): $(DOBJS)
	$(CC) $(CFLAGS) $(DFLAGS) $^ $(LFLAGS) -o $@

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

# ── CMake 経由（テスト・カバレッジ） ──
.PHONY: build
build:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. && make

.PHONY: test
test: build
	cd $(BUILD_DIR)/tests && make test

$(TEST_LOG): test

$(COV_INFO): $(TEST_LOG)
	lcov --capture --directory . --output-file $(COV_INFO)
	lcov --remove $(COV_INFO) '/usr/*' --output-file $(COV_INFO)

.PHONY: cov
cov: $(COV_INFO)
	lcov --list $(COV_INFO)

.PHONY: e2e
e2e: $(NAME)
	cd tests/e2e && sudo -E $$(command -v uv) run pytest -v

.PHONY: e2e-deps
e2e-deps:
	cd tests/e2e && uv sync
