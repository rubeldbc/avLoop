# AvLoop — the seamless loop maker for OBS. Root of the build (spec §4).
cmake_minimum_required(VERSION 3.28)
project(AvLoop VERSION 0.1.0 DESCRIPTION "Seamless looping WebM maker for OBS" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(AVLOOP_BUILD_TESTS "Build the unit tests" ON)
option(AVLOOP_ASAN "Build with AddressSanitizer (/fsanitize=address)" OFF)

# Every binary and test lands in build/bin, beside the Qt and FFmpeg DLLs.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(Warnings)
include(Qt)
find_package(FFmpeg REQUIRED)
include(Deploy)

configure_file(src/core/Version.h.in ${CMAKE_BINARY_DIR}/generated/AvLoopVersion.h @ONLY)

add_subdirectory(src)
if(AVLOOP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
