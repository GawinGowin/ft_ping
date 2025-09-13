# ICMP Packet Examples Makefile
# ft_pingプロジェクトのICMP構造体使用例

CC = cc
CFLAGS = -Wall -Wextra -Werror -std=c11 -g
INCLUDES = -I../cmd/ft_ping

# ターゲット
DGRAM_TARGET = dgram_example.out
RAW_TARGET = raw_example.out

# ソースファイル
DGRAM_SRC = icmp_dgram_example.c
RAW_SRC = icmp_raw_example.c

.PHONY: all clean dgram raw test help

all: $(DGRAM_TARGET) $(RAW_TARGET)

# 非特権DGRAM例のビルド
$(DGRAM_TARGET): $(DGRAM_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $<

# 特権RAW例のビルド  
$(RAW_TARGET): $(RAW_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $<

# DGRAMソケット例のみビルド
dgram: $(DGRAM_TARGET)

# RAWソケット例のみビルド
raw: $(RAW_TARGET)

# テスト実行（DGRAMのみ、非特権で実行可能）
test: $(DGRAM_TARGET)
	@echo "=== DGRAM ソケット テスト ==="
	@echo "非特権モードでlocalhostに対してテスト実行..."
	./$(DGRAM_TARGET) 127.0.0.1
	@echo
	@echo "=== RAW ソケット テスト ==="
	@echo "注意: RAWソケット例を実行するにはroot権限が必要です"
	@echo "実行方法: sudo ./$(RAW_TARGET) 127.0.0.1"

# テスト実行（root権限必要）
test-root: $(RAW_TARGET)
	@echo "=== RAW ソケット テスト（root権限） ==="
	@if [ "$$(id -u)" = "0" ]; then \
		echo "root権限でRAWソケットテスト実行..."; \
		./$(RAW_TARGET) 127.0.0.1; \
	else \
		echo "エラー: root権限が必要です"; \
		echo "実行方法: sudo make test-root"; \
		exit 1; \
	fi

# 外部ホストに対するテスト（Google DNS）
test-external: $(DGRAM_TARGET)
	@echo "=== 外部ホスト テスト ==="
	@echo "Google DNS (8.8.8.8) に対してテスト実行..."
	./$(DGRAM_TARGET) 8.8.8.8

# デバッグ情報付きビルド
debug: CFLAGS += -DDEBUG -O0
debug: all

# リリースビルド
release: CFLAGS += -O2 -DNDEBUG
release: all

# クリーンアップ
clean:
	rm -f *.o *.d

fclean: clean
	rm -f $(DGRAM_TARGET) $(RAW_TARGET)

re: fclean all

# ヘルプ
help:
	@echo "利用可能なターゲット:"
	@echo "  all         - 全ての例をビルド"
	@echo "  dgram       - DGRAM例のみビルド（非特権）"
	@echo "  raw         - RAW例のみビルド（特権必要）"
	@echo "  test        - DGRAM例のテスト実行"
	@echo "  test-root   - RAW例のテスト実行（sudo必要）"
	@echo "  test-external - 外部ホストに対するテスト"
	@echo "  debug       - デバッグビルド"
	@echo "  release     - リリースビルド"
	@echo "  clean       - ビルド成果物削除"
	@echo "  help        - このヘルプを表示"
	@echo
	@echo "使用例:"
	@echo "  make dgram"
	@echo "  make test"
	@echo "  sudo make test-root"
	@echo "  ./$(DGRAM_TARGET) 192.168.1.1"
	@echo "  sudo ./$(RAW_TARGET) 192.168.1.1"

# 依存関係の自動生成
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

# 依存関係のインクルード（エラーを無視）
-include $(DGRAM_SRC:.c=.d)
-include $(RAW_SRC:.c=.d)