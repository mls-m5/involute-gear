
cmake_minimum_required(VERSION 3.23)
project(involute-gears)

add_executable(
    involute-gears
    src/main.cpp
    )

target_compile_features(
    involute-gears
    PRIVATE
    cxx_std_17
    )

find_package(Threads)
target_link_libraries(
    involute-gears
    PRIVATE
    ${CMAKE_THREAD_LIBS_INIT}
    sdlpp
    )



if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
                cmake_policy(SET CMP0135 NEW)
endif()
include(FetchContent)
FetchContent_Declare(
    sdlpp
    GIT_REPOSITORY https://github.com/mls-m5/sdlpp)
FetchContent_MakeAvailable(sdlpp)
