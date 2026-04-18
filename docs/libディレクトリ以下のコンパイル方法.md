# lib/ 直下モジュールの CMake 組み込み方針

`lib/ping_stats.c` などの `lib/` 直下ファイルは、`liblib.a` を避けるため **OBJECT ライブラリ** としてビルドする。

**ルート `CMakeLists.txt`:**
```cmake
add_library(ping_stats OBJECT
  ${CMAKE_SOURCE_DIR}/lib/ping_stats.c
)
target_include_directories(ping_stats PUBLIC ${CMAKE_SOURCE_DIR}/lib)
target_compile_definitions(ping_stats PUBLIC TESTING=1)
```

**`tests/CMakeLists.txt`** の `target_link_libraries` に追加:
```cmake
target_link_libraries(InternalTests
  testftping vsock shared ping_stats
  GTest::GTest GTest::Main
)
```

CMake 3.12 以降は OBJECT ライブラリを `target_link_libraries` に直接渡せるため、`$<TARGET_OBJECTS:ping_stats>` のようなジェネレータ式は不要。

テストファイルのインクルード:
```cpp
extern "C" {
#include "ping_stats.h"
}
```

`target_include_directories` に `${CMAKE_SOURCE_DIR}/lib` を追加すること。

---

## 変更ファイル一覧

| ファイル                  | 変更内容                                           |
| ------------------------- | -------------------------------------------------- |
| `cmd/ft_ping/icmp.c`      | `calculate_checksum` の `static` を外す            |
| `cmd/ft_ping/icmp.h`      | `calculate_checksum` の宣言を追加                  |
| `cmd/ft_ping/ft_ping.h`   | `t_socket_st` 定義を削除、`vsock.h` をインクルード |
| `cmd/ft_ping/infra.c`     | `create_socket` を削除                             |
| `cmd/ft_ping/usecases.c`  | Step 5, 6, 7 の置換（3箇所）                       |
| `cmd/ft_ping/ft_ping.c`   | Step 8 の置換（1箇所）                             |
| `lib/vsock/vsock_raw.c`   | `extract_icmp_raw` を正しく実装                    |
| `lib/vsock/vsock_dgram.c` | `extract_icmp_dgram` を正しく実装                  |