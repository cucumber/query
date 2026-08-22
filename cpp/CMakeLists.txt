# The VERSION here is a placeholder; the real version is in the VERSION file.
set(VERSION 1.0.0)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    file(STRINGS "VERSION" LINES)
    list(GET LINES 0 VERSION)
endif()

# Crude Semver parsing
if("${VERSION}" MATCHES "^([^\\.]+)\\.([^\\.]+)\\.([^\\.]+)$")
    set(VER_MAJOR ${CMAKE_MATCH_1})
    set(VER_MINOR ${CMAKE_MATCH_2})
    set(VER_PATCH ${CMAKE_MATCH_3})
else()
    message(FATAL_ERROR "unable to parse version: ${VERSION}")
endif()

cmake_minimum_required(VERSION 3.21 FATAL_ERROR)
project(cucumber_query VERSION ${VERSION} LANGUAGES CXX)

# Options
if (POLICY CMP0077)
    # Allow CMake 3.13+ to override options when using FetchContent / add_subdirectory.
    cmake_policy(SET CMP0077 NEW)
endif ()

option(CUCUMBER_QUERY_BUILD_TESTS "Build the unit tests." Off)
option(CUCUMBER_QUERY_ENABLE_CLANG_TIDY "Run clang-tidy as part of the build." Off)
option(CUCUMBER_QUERY_ENABLE_COVERAGE "Enable compiler flags for code coverage measurements." Off)
option(CUCUMBER_QUERY_ENABLE_SANITIZERS "Enable compiler flags for the address and undefined behaviour sanitizers." Off)

if(CUCUMBER_QUERY_ENABLE_SANITIZERS AND CUCUMBER_QUERY_ENABLE_COVERAGE)
    message(FATAL_ERROR "CUCUMBER_QUERY_ENABLE_SANITIZERS and CUCUMBER_QUERY_ENABLE_COVERAGE are mutually exclusive.")
endif()

if(CUCUMBER_QUERY_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_PROGRAM clang-tidy REQUIRED)
endif()

if(CUCUMBER_QUERY_ENABLE_COVERAGE)
    find_program(GCOVR_PROGRAM gcovr REQUIRED)

    add_custom_target(
        generate-lcov-report
        COMMAND ${GCOVR_PROGRAM} --version
        COMMAND ${GCOVR_PROGRAM} --root ${CMAKE_CURRENT_SOURCE_DIR} --merge-lines --delete --exclude-unreachable-branches --exclude-throw-branches --exclude-noncode-lines --filter include/ --filter src/
            --txt-summary
            --lcov ${CMAKE_CURRENT_BINARY_DIR}/lcov.info
            --cobertura ${CMAKE_CURRENT_BINARY_DIR}/cobertura.xml --cobertura-pretty
            --markdown-summary ${CMAKE_CURRENT_BINARY_DIR}/coverage-summary.md
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating code coverage reports"
    )
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler cache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
else()
    find_program(SCCACHE_PROGRAM sccache)
    if(SCCACHE_PROGRAM)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
        message(STATUS "Using sccache: ${SCCACHE_PROGRAM}")
    endif()
endif()

# Configuration
include(GNUInstallDirs)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS)
endif()

# CMake dependencies
find_package(nlohmann_json REQUIRED)
find_package(cucumber_messages QUIET)
if (NOT cucumber_messages_FOUND AND NOT TARGET cucumber::messages)
    message(FATAL_ERROR "cucumber::messages not found")
endif()

# Target options
if(CUCUMBER_QUERY_ENABLE_CLANG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_PROGRAM}")
endif()

if(CUCUMBER_QUERY_ENABLE_COVERAGE)
    add_compile_options(
        -g -O0 --coverage -fprofile-arcs -ftest-coverage -fno-inline
        $<$<COMPILE_LANGUAGE:CXX>:-fno-elide-constructors>
    )

    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        link_libraries(gcov)
    else()
        add_link_options(--coverage)
    endif()
endif()

if(CUCUMBER_QUERY_ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
endif()

# Targets
add_library(cucumber_query_lib)
add_library(cucumber::query ALIAS cucumber_query_lib)

target_sources(
    cucumber_query_lib
    PRIVATE
        include/cucumber/query/Lineage.hpp
        include/cucumber/query/Query.hpp
        src/query/Lineage.cpp
        src/query/Query.cpp
        include/cucumber/query/NamingStrategy.hpp
        src/query/NamingStrategy.cpp
)

target_include_directories(
    cucumber_query_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/query
)

target_link_libraries(
    cucumber_query_lib
    PUBLIC
        nlohmann_json::nlohmann_json
        cucumber::messages
)

set_target_properties(
    cucumber_query_lib
    PROPERTIES
        VERSION ${VERSION}
        SOVERSION ${VER_MAJOR}
        EXPORT_NAME query
        OUTPUT_NAME cucumber_query
)

# Installation support
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "cmake/cucumber_query-config.cmake.in"
    "${PROJECT_BINARY_DIR}/cucumber_query-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cucumber_query"
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
)

write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/cucumber_query-config-version.cmake"
    COMPATIBILITY AnyNewerVersion
)

install(
    TARGETS cucumber_query_lib
    EXPORT cucumber_query-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    EXPORT cucumber_query-targets
    FILE cucumber_query-targets.cmake
    NAMESPACE cucumber::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cucumber_query"
)

install(
    FILES
        "${PROJECT_BINARY_DIR}/cucumber_query-config.cmake"
        "${PROJECT_BINARY_DIR}/cucumber_query-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cucumber_query"
)

install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Optional: tests
if(CUCUMBER_QUERY_BUILD_TESTS)
    find_package(GTest REQUIRED)
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif()
