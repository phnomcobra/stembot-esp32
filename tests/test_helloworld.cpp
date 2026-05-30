#include <catch2/catch_test_macros.hpp>
#include "helloworld.h"

TEST_CASE("helloworld::greet constructs greeting correctly", "[helloworld]")
{
    SECTION("standard name")
    {
        REQUIRE(helloworld::greet("World") == "Hello, World!");
    }

    SECTION("custom name")
    {
        REQUIRE(helloworld::greet("ESP32") == "Hello, ESP32!");
    }

    SECTION("empty name")
    {
        REQUIRE(helloworld::greet("") == "Hello, !");
    }
}
