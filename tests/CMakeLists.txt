include(GoogleTest)

file(GLOB_RECURSE TEST_SOURCES
"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

#..:: InternalTests ::..
add_executable(InternalTests ${TEST_SOURCES})

if(GTest_FOUND)
  target_link_libraries(InternalTests testftping GTest::GTest GTest::Main)
else()
  target_link_libraries(InternalTests testftping gtest gtest_main)
endif()

target_include_directories(InternalTests PRIVATE 
  ${CMAKE_SOURCE_DIR}/cmd/ft_ping
  ${gtest_SOURCE_DIR}
)

# ..:: GoogleTest ::..
gtest_add_tests(TARGET InternalTests)
