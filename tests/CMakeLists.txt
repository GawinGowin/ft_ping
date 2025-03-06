include(GoogleTest)

#..:: InternalTests ::..
add_executable(InternalTests 
 ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp
)

target_link_libraries(InternalTests testftping gtest_main)
target_include_directories(InternalTests PRIVATE 
  ${CMAKE_SOURCE_DIR}/cmd/ft_ping
  ${gtest_SOURCE_DIR}
)

# ..:: GoogleTest ::..
gtest_add_tests(TARGET InternalTests)
