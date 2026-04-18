cmake_minimum_required(VERSION 3.25.1)
project(ft_ping)

set(CMAKE_C_COMPILER /usr/bin/cc)
set(CMAKE_CXX_COMPILER /usr/bin/c++)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -g3 -fsanitize=address -fprofile-arcs -ftest-coverage -O0")
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror -g3 -fsanitize=address -fprofile-arcs -ftest-coverage -O0")

## Sources
file(GLOB_RECURSE SOURCES
"${CMAKE_SOURCE_DIR}/cmd/ft_ping/*.c"
)

message(STATUS "SOURCES: ${SOURCES}")

add_library(testftping ${SOURCES}) 
target_include_directories(
  testftping
  PRIVATE ${CMAKE_SOURCE_DIR}/cmd/ft_ping
)
target_compile_definitions(testftping PUBLIC TESTING=1)

# add_subdirectory(lib/shared)
add_subdirectory(lib/vsock)

add_executable(ft_ping ${SOURCES})

target_include_directories(
  ft_ping
  PRIVATE ${CMAKE_SOURCE_DIR}/cmd/ft_ping
)

target_link_libraries(ft_ping PRIVATE vsock)
# target_link_libraries(ft_ping PRIVATE vsock shared)

enable_testing()
add_subdirectory(tests)
