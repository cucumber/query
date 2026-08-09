# Verifies that every query type and naming strategy variant in testdata/src
# has a corresponding TEST_P in the C++ acceptance tests.
#
# Required variables (passed via -D):
#   TEST_BINARY   — path to the built cucumber_query_lib.test binary
#   TESTDATA_SRC  — path to testdata/src

execute_process(
    COMMAND "${TEST_BINARY}" --gtest_list_tests
    OUTPUT_VARIABLE raw_output
    RESULT_VARIABLE result_code
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT result_code EQUAL 0)
    message(FATAL_ERROR "Failed to list tests: ${TEST_BINARY} --gtest_list_tests")
endif()

string(REPLACE "\r\n" "\n" raw_output "${raw_output}")
string(REPLACE "\n" ";" lines "${raw_output}")

set(in_query_suite FALSE)
set(in_naming_suite FALSE)
set(cpp_query_names "")
set(cpp_naming_variants "")

foreach(line IN LISTS lines)
    if(line MATCHES "^Acceptance/QueryAcceptanceTest\\.")
        set(in_query_suite TRUE)
        set(in_naming_suite FALSE)
    elseif(line MATCHES "^Acceptance/NamingStrategyAcceptanceTest\\.")
        set(in_query_suite FALSE)
        set(in_naming_suite TRUE)
    elseif(line MATCHES "^[^ ]")
        set(in_query_suite FALSE)
        set(in_naming_suite FALSE)
    elseif(in_query_suite AND line MATCHES "^  [^ ]")
        string(REGEX REPLACE "^  ([^ /]+)/.*$" "\\1" test_name "${line}")
        list(APPEND cpp_query_names "${test_name}")
    elseif(in_naming_suite AND line MATCHES "^  [^ ]")
        string(REGEX REPLACE "^  ([^ /]+)/.*$" "\\1" test_name "${line}")
        # gtest uses underscores in test names; testdata file variants use hyphens
        string(REPLACE "_" "-" variant_name "${test_name}")
        list(APPEND cpp_naming_variants "${variant_name}")
    endif()
endforeach()

list(REMOVE_DUPLICATES cpp_query_names)
list(REMOVE_DUPLICATES cpp_naming_variants)

file(GLOB result_files "${TESTDATA_SRC}/*.results.json")
set(testdata_query_names "")
foreach(f IN LISTS result_files)
    get_filename_component(filename "${f}" NAME)
    string(REGEX REPLACE "^[^.]+\\.(.+)\\.results\\.json$" "\\1" query_name "${filename}")
    list(APPEND testdata_query_names "${query_name}")
endforeach()
list(REMOVE_DUPLICATES testdata_query_names)

file(GLOB naming_files "${TESTDATA_SRC}/*.naming-strategy.*.txt")
set(testdata_naming_variants "")
foreach(f IN LISTS naming_files)
    get_filename_component(filename "${f}" NAME)
    string(REGEX REPLACE "^[^.]+\\.naming-strategy\\.(.+)\\.txt$" "\\1" variant "${filename}")
    list(APPEND testdata_naming_variants "${variant}")
endforeach()
list(REMOVE_DUPLICATES testdata_naming_variants)

set(found_gaps FALSE)

foreach(name IN LISTS testdata_query_names)
    if(NOT name IN_LIST cpp_query_names)
        message(WARNING "Query type '${name}' exists in testdata but has no TEST_P in QueryAcceptanceTest")
        set(found_gaps TRUE)
    endif()
endforeach()

foreach(variant IN LISTS testdata_naming_variants)
    if(NOT variant IN_LIST cpp_naming_variants)
        message(WARNING "Naming strategy variant '${variant}' exists in testdata but has no TEST_P in NamingStrategyAcceptanceTest")
        set(found_gaps TRUE)
    endif()
endforeach()

if(found_gaps)
    message(FATAL_ERROR "C++ tests are missing coverage for new testdata. Add TEST_P entries to TestQuery.cpp.")
endif()

message(STATUS "All testdata query types and naming strategy variants are covered.")
