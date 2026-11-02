cmake_minimum_required(VERSION 3.20)
project(encoding-stub LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

set(BUILD_VERSION "" CACHE STRING "Version of the plugin being built")
if("${BUILD_VERSION}" STREQUAL "")
    message(SEND_ERROR "Missing or empty BUILD_VERSION argument")
endif()

option(BUILD_UNIT_TESTS "Build encoding component unit tests" OFF)

add_subdirectory(source)

if(BUILD_UNIT_TESTS)
    enable_testing()
    add_subdirectory(test/source)
endif()