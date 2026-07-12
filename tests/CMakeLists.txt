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

# Conan provides Catch2 and MbedTLS via the toolchain file injected by CMakePresets.json
find_package(Catch2 REQUIRED)
find_package(MbedTLS REQUIRED)

# ArduinoJson — header-only; reuse the copy already fetched by the ESP-IDF
# Component Manager so the native tests stay in sync with the firmware build.
# Both the root dir (for ArduinoJson.h) and src/ (for <ArduinoJson/...>
# angle-bracket sub-includes) must be on the include path.
set(ARDUINOJSON_ROOT
    ${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/bblanchon__arduinojson)
set(ARDUINOJSON_INCLUDE ${ARDUINOJSON_ROOT} ${ARDUINOJSON_ROOT}/src)

# Compile the models component (control + network + utils) as a plain static library
add_library(models STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/models/control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/models/network.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/models/utils.cpp
)
target_include_directories(models PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/models/include
    ${ARDUINOJSON_INCLUDE}
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

# Compile the agent crypto helpers (AES-256-EAX + set_isrc_json) as a static
# library for the host.  Only aes_eax.cpp is compiled here — agent.cpp depends
# on esp_http_client which is ESP-IDF-only.
add_library(agent_crypto STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/executor/agent/aes_eax.cpp
)
target_include_directories(agent_crypto PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/executor/agent/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/models/include
    ${ARDUINOJSON_INCLUDE}
)
target_link_libraries(agent_crypto PUBLIC MbedTLS::mbedtls models)

# Test executable — network message serialisation / deserialisation
add_executable(test_network
    test_network.cpp
)
target_link_libraries(test_network
    PRIVATE
        Catch2::Catch2WithMain
        models
)

# Test executable — AgentClient helpers (crypto + set_isrc + canonical JSON)
add_executable(test_agent
    test_agent.cpp
)
target_link_libraries(test_agent
    PRIVATE
        Catch2::Catch2WithMain
        agent_crypto
        models
)

# Wire Catch2 test discovery into CTest
include(CTest)
include(Catch)
catch_discover_tests(test_control)
catch_discover_tests(test_network)
catch_discover_tests(test_agent)
