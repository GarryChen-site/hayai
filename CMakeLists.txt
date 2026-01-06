cmake_minimum_required(VERSION 3.20)
project(hayai VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags for macOS ARM64
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

#Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

#Library
file(GLOB_RECURSE SOURCES "src/*.cc")
add_library(hayai STATIC ${SOURCES})

#Tests
enable_testing()
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
file(GLOB_RECURSE TEST_SOURCES "tests/*.cc")

foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} hayai GTest::gtest_main)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

#Examples
add_executable(echo_server examples/echo_server.cc)
target_link_libraries(echo_server hayai)