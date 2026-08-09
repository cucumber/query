add_executable(cucumber_query_lib.test)

target_sources(cucumber_query_lib.test
    PRIVATE
        TestQuery.cpp
)

target_link_libraries(cucumber_query_lib.test
    PRIVATE
        cucumber::query
        GTest::gmock_main
)

target_compile_definitions(cucumber_query_lib.test
    PRIVATE
        TESTDATA_SRC="${CMAKE_SOURCE_DIR}/../testdata/src"
)

include(GoogleTest)
gtest_discover_tests(cucumber_query_lib.test)

add_custom_target(
    check-testdata-coverage
    COMMAND ${CMAKE_COMMAND}
        -DTEST_BINARY=$<TARGET_FILE:cucumber_query_lib.test>
        -DTESTDATA_SRC=${CMAKE_SOURCE_DIR}/../testdata/src
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckTestdataCoverage.cmake
    DEPENDS cucumber_query_lib.test
    COMMENT "Checking testdata coverage in C++ tests"
)
