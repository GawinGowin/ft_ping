cmake_minimum_required(VERSION 3.25.1)
project(ft_ping)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -g3 -fsanitize=address -fprofile-arcs -ftest-coverage -O0")
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror -g3 -fsanitize=address -fprofile-arcs -ftest-coverage -O0")

## Sources
file(GLOB_RECURSE SOURCES
"${CMAKE_SOURCE_DIR}/cmd/ft_ping/*.c"
)

file(GLOB_RECURSE LIB_SOURCES
"${CMAKE_SOURCE_DIR}/cmd/ft_ping/*.c"
)

list(REMOVE_ITEM LIB_SOURCES "${CMAKE_SOURCE_DIR}/cmd/ft_ping/ft_ping.c")

message(STATUS "SOURCES: ${LIB_SOURCES}")
add_library(testftping ${LIB_SOURCES}) 

target_include_directories(
  testftping
  PRIVATE ${CMAKE_SOURCE_DIR}/cmd/ft_ping
)

## Link with necessary libraries
# set(LIBRARIES 
# ...
# )
# target_link_libraries(testftping ${LIBRARIES})

add_executable(ft_ping ${SOURCES})

target_include_directories(
  ft_ping
  PRIVATE ${CMAKE_SOURCE_DIR}/cmd/ft_ping
)
target_link_libraries(ft_ping testftping ${LIBRARIES})

## install gtest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
## For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
FetchContent_GetProperties(googletest)

enable_testing()
add_subdirectory(tests)