find_package(GTest QUIET)
if(NOT GTest_FOUND)
  ## install gtest
  message(STATUS "GTest not found, fetching it")
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  ## For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  FetchContent_GetProperties(googletest)
else()
  message(STATUS "GTest found")
endif()

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
