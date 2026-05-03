file(GLOB_RECURSE SOURCES
"${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_library(vsock STATIC ${SOURCES})
add_library(vsock_testing STATIC ${SOURCES})
target_compile_definitions(vsock_testing PUBLIC TESTING=1)

foreach(tgt vsock vsock_testing)
  target_include_directories(${tgt} PUBLIC ${CMAKE_SOURCE_DIR}/lib)
endforeach()
