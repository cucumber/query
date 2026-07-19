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
option(CUCUMBER_QUERY_FETCH_DEPS "Fetch dependencies via FetchContent." Off)

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
include(cmake/dependencies.cpm.cmake)

# Target
add_library(cucumber_query_lib)
add_library(cucumber::query ALIAS cucumber_query_lib)

target_sources(
    cucumber_query_lib
    PRIVATE
        include/cucumber/query/Query.hpp
        src/query/Query.cpp
)

target_include_directories(
    cucumber_query_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/cucumber>
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/messages
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

# Installation
# Installation
# Installation
# Installation
# Installation
# Installation
# Installation

# Optional: tests
if(CUCUMBER_QUERY_BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif()
