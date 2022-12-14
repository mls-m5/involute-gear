
cmake_minimum_required(VERSION 3.23)
project(involute-gears)

add_library(
    common
    INTERFACE
    )


target_compile_features(
    common
    INTERFACE
    cxx_std_17
    )

find_package(Threads)
target_link_libraries(
    common
    INTERFACE
    ${CMAKE_THREAD_LIBS_INIT}
    sdlpp
    )

add_executable(
    involute-gears
    src/main.cpp
    )

target_link_libraries(
    involute-gears
    PRIVATE
    common
    )

add_executable(
    first-test
    src/first-test.cpp
    )

target_link_libraries(
    first-test
    PRIVATE
    common
    )


if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
                cmake_policy(SET CMP0135 NEW)
endif()
include(FetchContent)
FetchContent_Declare(
    sdlpp
    GIT_REPOSITORY https://github.com/mls-m5/sdlpp)
FetchContent_MakeAvailable(sdlpp)
