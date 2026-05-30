cmake_minimum_required(VERSION 3.22)
project(stembot-tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optional clang-tidy static analysis — enabled by the native-lint CMake preset.
option(ENABLE_CLANG_TIDY "Run clang-tidy on all compiled sources" OFF)
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy REQUIRED
        DOC "Path to the clang-tidy executable")
    message(STATUS "clang-tidy: ${CLANG_TIDY_EXE}")
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
endif()

# Conan provides Catch2 via the toolchain file injected by CMakePresets.json
find_package(Catch2 REQUIRED)

# Compile the helloworld component as a plain static library for the host
add_library(helloworld STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/helloworld/helloworld.cpp
)
target_include_directories(helloworld PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/helloworld/include
)

# Test executable
add_executable(test_helloworld
    test_helloworld.cpp
)
target_link_libraries(test_helloworld
    PRIVATE
        Catch2::Catch2WithMain
        helloworld
)

# Wire Catch2 test discovery into CTest
include(CTest)
include(Catch)
catch_discover_tests(test_helloworld)
