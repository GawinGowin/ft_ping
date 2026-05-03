file(GLOB TOOL_CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tool_*.c")
list(REMOVE_ITEM TOOL_CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tool_main.c")

add_library(tool_core STATIC ${TOOL_CORE_SOURCES})
add_library(tool_core_testing STATIC ${TOOL_CORE_SOURCES})
target_compile_definitions(tool_core_testing PUBLIC TESTING=1)

foreach(tgt tool_core tool_core_testing)
  target_include_directories(${tgt} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/ft_ping
    ${CMAKE_SOURCE_DIR}/lib
  )
endforeach()

add_executable(ft_ping ${CMAKE_CURRENT_SOURCE_DIR}/tool_main.c)
target_include_directories(ft_ping PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include/ft_ping
)
target_link_libraries(ft_ping PRIVATE tool_core ftping_core)
