file(GLOB_RECURSE SOURCES
"${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_library(shared STATIC ${SOURCES})

target_include_directories(
  shared
  PUBLIC ${CMAKE_SOURCE_DIR}/lib
)
