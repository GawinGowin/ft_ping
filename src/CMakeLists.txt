add_library(tool_getparam OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/tool_getparam.c)
target_include_directories(tool_getparam PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include/ft_ping
  ${CMAKE_SOURCE_DIR}/lib
)
target_compile_definitions(tool_getparam PUBLIC TESTING=1)

add_library(tool_signal OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/tool_signal.c)
target_include_directories(tool_signal PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include/ft_ping
)
target_compile_definitions(tool_signal PUBLIC TESTING=1)

add_library(tool_output OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/tool_output.c)
target_include_directories(tool_output PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/ft_ping
)
target_compile_definitions(tool_output PUBLIC TESTING=1)

add_library(tool_cleanup OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/tool_cleanup.c)
target_include_directories(tool_cleanup PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include/ft_ping
)
target_compile_definitions(tool_cleanup PUBLIC TESTING=1)

add_executable(tool_ft_ping ${CMAKE_CURRENT_SOURCE_DIR}/tool_main.c)
target_include_directories(tool_ft_ping PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include/ft_ping
)
target_link_libraries(tool_ft_ping PRIVATE tool_getparam tool_signal tool_output tool_cleanup ftping ping_loop ping_stats ping_config ping_icmp ping_schedule vsock shared)
