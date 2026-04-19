add_library(tool_getparam OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/tool_getparam.c)
target_include_directories(tool_getparam PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include/ft_ping
  ${CMAKE_SOURCE_DIR}/lib
)
target_compile_definitions(tool_getparam PUBLIC TESTING=1)
