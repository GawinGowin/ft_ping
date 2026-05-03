file(GLOB_RECURSE SOURCES
"${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_library(shared STATIC ${SOURCES})
add_library(shared_testing STATIC ${SOURCES})
target_compile_definitions(shared_testing PUBLIC TESTING=1)

foreach(tgt shared shared_testing)
  target_include_directories(${tgt} PUBLIC ${CMAKE_SOURCE_DIR}/lib)
endforeach()
