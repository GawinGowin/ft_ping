add_subdirectory(shared)
add_subdirectory(vsock)

file(GLOB FTPING_CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(ftping_core STATIC ${FTPING_CORE_SOURCES})
add_library(ftping_core_testing STATIC ${FTPING_CORE_SOURCES})
target_compile_definitions(ftping_core_testing PUBLIC TESTING=1)

target_include_directories(ftping_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ftping_core PUBLIC vsock shared)
target_link_libraries(vsock PUBLIC "$<LINK_ONLY:ftping_core>")

target_include_directories(ftping_core_testing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ftping_core_testing PUBLIC vsock_testing shared_testing)
target_link_libraries(vsock_testing PUBLIC "$<LINK_ONLY:ftping_core_testing>")
