file(GLOB INTEGRATION_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(IntegrationTests
  ${INTEGRATION_SOURCES}
  ${CMAKE_SOURCE_DIR}/tests/test_main.cpp)

if(GTest_FOUND)
  target_link_libraries(IntegrationTests ${FTPING_LIBS} GTest::GTest GTest::Main)
else()
  target_link_libraries(IntegrationTests ${FTPING_LIBS} gtest gtest_main)
endif()

target_include_directories(IntegrationTests PRIVATE
  ${CMAKE_SOURCE_DIR}/cmd/ft_ping
  ${CMAKE_SOURCE_DIR}/lib
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/ft_ping
  ${gtest_SOURCE_DIR})

gtest_discover_tests(IntegrationTests PROPERTIES LABELS "integration")
