add_subdirectory(shared)
add_subdirectory(vsock)

foreach(module ping_stats ping_icmp ping_config ping_loop ping_schedule)
  add_library(${module} OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/${module}.c)
  target_include_directories(${module} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_definitions(${module} PUBLIC TESTING=1)
endforeach()
