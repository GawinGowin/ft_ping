cmake_minimum_required(VERSION 3.25.1)
project(ft_ping)

set(CMAKE_C_STANDARD 11)
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
target_link_libraries(ft_ping ${LIBRARIES})

find_package(GTest QUIET)
if(NOT GTest_FOUND)
  ## install gtest
  message(STATUS "GTest not found, fetching it")
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
else()
  message(STATUS "GTest found")
endif()

enable_testing()
add_subdirectory(tests)