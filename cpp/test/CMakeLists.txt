add_executable(cucumber_query_lib.test)
# add_test(NAME cucumber_query_lib.test COMMAND cucumber_query_lib.test)

target_sources(cucumber_query_lib.test
    PRIVATE
        TestQuery.cpp
)

target_link_libraries(cucumber_query_lib.test
    PRIVATE
        cucumber::query
        GTest::gmock
)

target_compile_definitions(cucumber_query_lib.test
    PRIVATE
        TESTDATA_SRC="${CMAKE_SOURCE_DIR}/../testdata/src"
)

include(GoogleTest)
gtest_discover_tests(cucumber_query_lib.test)
