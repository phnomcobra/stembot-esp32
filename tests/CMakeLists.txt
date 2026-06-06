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

# ArduinoJson — header-only; reuse the copy already fetched by the ESP-IDF
# Component Manager so the native tests stay in sync with the firmware build.
# Both the root dir (for ArduinoJson.h) and src/ (for <ArduinoJson/...>
# angle-bracket sub-includes) must be on the include path.
set(ARDUINOJSON_ROOT
    ${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/bblanchon__arduinojson)
set(ARDUINOJSON_INCLUDE ${ARDUINOJSON_ROOT} ${ARDUINOJSON_ROOT}/src)

# Compile the helloworld component as a plain static library for the host
add_library(helloworld STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/helloworld/helloworld.cpp
)
target_include_directories(helloworld PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/helloworld/include
)

# Compile the models component (control + utils) as a plain static library
add_library(models STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/models/control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/models/utils.cpp
)
target_include_directories(models PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/models/include
    ${ARDUINOJSON_INCLUDE}
)

# Test executable — helloworld
add_executable(test_helloworld
    test_helloworld.cpp
)
target_link_libraries(test_helloworld
    PRIVATE
        Catch2::Catch2WithMain
        helloworld
)

# Test executable — control form serialisation / deserialisation
add_executable(test_control
    test_control.cpp
)
target_link_libraries(test_control
    PRIVATE
        Catch2::Catch2WithMain
        models
)

# Wire Catch2 test discovery into CTest
include(CTest)
include(Catch)
catch_discover_tests(test_helloworld)
catch_discover_tests(test_control)
