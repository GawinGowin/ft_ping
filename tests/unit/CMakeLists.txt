file(GLOB UNIT_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/mock/*.cpp")

add_executable(UnitTests
  ${UNIT_SOURCES}
  ${CMAKE_SOURCE_DIR}/tests/test_main.cpp)

if(GTest_FOUND)
  target_link_libraries(UnitTests ${FTPING_LIBS} GTest::GTest GTest::Main)
else()
  target_link_libraries(UnitTests ${FTPING_LIBS} gtest gtest_main)
endif()

target_include_directories(UnitTests PRIVATE
  ${CMAKE_SOURCE_DIR}/cmd/ft_ping
  ${CMAKE_SOURCE_DIR}/lib
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/ft_ping
  ${CMAKE_CURRENT_SOURCE_DIR}/mock
  ${gtest_SOURCE_DIR})

gtest_discover_tests(UnitTests PROPERTIES LABELS "unit")
