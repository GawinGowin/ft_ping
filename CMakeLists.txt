cmake_minimum_required(VERSION 3.25.1)
project(ft_ping)

set(CMAKE_C_COMPILER /usr/bin/cc)
set(CMAKE_CXX_COMPILER /usr/bin/c++)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -g3 -fsanitize=address -fprofile-arcs -ftest-coverage -O0")
set(CMAKE_C_FLAGS   "-Wall -Wextra -Werror -g3 -fsanitize=address -fprofile-arcs -ftest-coverage -O0")

add_subdirectory(lib)
add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
