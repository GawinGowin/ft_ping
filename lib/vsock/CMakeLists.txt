file(GLOB_RECURSE SOURCES
"${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_library(vsock STATIC ${SOURCES})

target_include_directories(
  vsock
  PUBLIC ${CMAKE_SOURCE_DIR}/lib
)
