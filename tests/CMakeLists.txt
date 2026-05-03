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

option(ENABLE_INTEGRATION_TESTS "Enable integration tests (requires CAP_NET_RAW)" OFF)

set(FTPING_LIBS
  ftping_core_testing tool_core_testing
  CACHE INTERNAL "ft_ping libraries to link in tests"
)

add_subdirectory(unit)
add_subdirectory(tool)
if(ENABLE_INTEGRATION_TESTS)
  add_subdirectory(integration)
endif()
