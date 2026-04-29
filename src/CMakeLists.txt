file(GLOB TOOL_CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tool_*.c")
list(REMOVE_ITEM TOOL_CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tool_main.c")

add_library(tool_core STATIC ${TOOL_CORE_SOURCES})
target_include_directories(tool_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/ft_ping
  ${CMAKE_SOURCE_DIR}/lib
)
target_compile_definitions(tool_core PUBLIC TESTING=1)

add_executable(tool_ft_ping ${CMAKE_CURRENT_SOURCE_DIR}/tool_main.c)
target_include_directories(tool_ft_ping PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include/ft_ping
)
target_link_libraries(tool_ft_ping PRIVATE tool_core ftping_core)
