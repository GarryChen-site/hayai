cmake_minimum_required(VERSION 3.20)
project(hayai VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags for macOS ARM64
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
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

# Test executables
enable_testing()

add_executable(InetAddressTest tests/InetAddressTest.cc)
target_link_libraries(InetAddressTest hayai gtest_main)
add_test(NAME InetAddressTest COMMAND InetAddressTest)

add_executable(SocketTest tests/SocketTest.cc)
target_link_libraries(SocketTest hayai gtest_main)
add_test(NAME SocketTest COMMAND SocketTest)

add_executable(EventLoopTest tests/EventLoopTest.cc)
target_link_libraries(EventLoopTest hayai gtest_main)
add_test(NAME EventLoopTest COMMAND EventLoopTest)

add_executable(AcceptorTest tests/AcceptorTest.cc)
target_link_libraries(AcceptorTest hayai gtest_main)
add_test(NAME AcceptorTest COMMAND AcceptorTest)

add_executable(BufferTest tests/BufferTest.cc)
target_link_libraries(BufferTest hayai gtest_main)
add_test(NAME BufferTest COMMAND BufferTest)

add_executable(TcpConnectionTest tests/TcpConnectionTest.cc)
target_link_libraries(TcpConnectionTest hayai gtest_main)
add_test(NAME TcpConnectionTest COMMAND TcpConnectionTest)