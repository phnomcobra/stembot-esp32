// Protocol-compatibility tests for the GetConfig and Benchmark
// control forms.
//
// Canonical JSON strings are taken directly from the Rust reference
// implementation (stembot-rust/src/models/control.rs) to enforce wire
// compatibility.  Field ordering within an object is intentionally
// irrelevant — each test parses the serialised output with ArduinoJson and
// inspects individual fields, matching the order-insensitive strategy used
// by the Rust assert_ser_eq / assert_deser_roundtrip helpers.

#include "control.hpp"

#include <ArduinoJson.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

// ── canonical wire JSON from stembot-rust/src/models/control.rs ──────────────

// GetConfig — from Rust GET_CONFIG_REQUEST_JSON / GET_CONFIG_RESPONSE_JSON
static constexpr const char* GET_CONFIG_REQUEST_JSON =
    R"({"type":"get_config","error":null,"objuuid":null,"coluuid":null,"config":null})";
static constexpr const char* GET_CONFIG_RESPONSE_JSON =
    R"({"type":"get_config","error":null,"objuuid":null,"coluuid":null,"config":{"agtuuid":"a1","port":8080}})";
static constexpr const char* GET_CONFIG_ERROR_JSON =
    R"({"type":"get_config","error":"not found","objuuid":"o1","coluuid":"c1","config":null})";

// Benchmark — the Rust implementation defines the struct but has no test
// constants yet; the wire format is derived from the tagged-enum definition
// (tag "benchmark", fields: outbound_size, inbound_size, payload, error,
// objuuid, coluuid — all Option).
static constexpr const char* BENCHMARK_REQUEST_JSON =
    R"({"type":"benchmark","outbound_size":1024,"inbound_size":null,"payload":null,"error":null,"objuuid":null,"coluuid":null})";
static constexpr const char* BENCHMARK_RESPONSE_JSON =
    R"({"type":"benchmark","outbound_size":1024,"inbound_size":512,"payload":"aGVsbG8=","error":null,"objuuid":null,"coluuid":null})";
static constexpr const char* BENCHMARK_ERROR_JSON =
    R"({"type":"benchmark","outbound_size":null,"inbound_size":null,"payload":null,"error":"timeout","objuuid":"o1","coluuid":"c1"})";

// ── helpers ───────────────────────────────────────────────────────────────────

// Parse a JSON string; REQUIRE parse succeeds.
static JsonDocument parse(const std::string& json)
{
    JsonDocument doc;
    auto err = deserializeJson(doc, json);
    REQUIRE(!err);
    return doc;
}

// ═════════════════════════════════════════════════════════════════════════════
// GetConfig
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("GetConfig::to_json — request (config null)", "[control][get_config]")
{
    GetConfig f;
    JsonDocument doc = parse(f.to_json());

    REQUIRE(doc["type"] == "get_config");
    REQUIRE(doc["config"].isNull());
    REQUIRE(doc["error"].isNull());
    REQUIRE(doc["objuuid"].isNull());
    REQUIRE(doc["coluuid"].isNull());
}

TEST_CASE("GetConfig::to_json — response (config present)", "[control][get_config]")
{
    GetConfig f;
    f.config_json = R"({"agtuuid":"a1","port":8080})";

    JsonDocument doc = parse(f.to_json());

    REQUIRE(doc["type"] == "get_config");
    REQUIRE(doc["config"]["agtuuid"] == "a1");
    REQUIRE(doc["config"]["port"] == 8080);
    REQUIRE(doc["error"].isNull());
    REQUIRE(doc["objuuid"].isNull());
    REQUIRE(doc["coluuid"].isNull());
}

TEST_CASE("GetConfig::to_json — error response", "[control][get_config]")
{
    GetConfig f;
    f.error = "not found";
    f.objuuid = "o1";
    f.coluuid = "c1";

    JsonDocument doc = parse(f.to_json());

    REQUIRE(doc["type"] == "get_config");
    REQUIRE(doc["config"].isNull());
    REQUIRE(doc["error"] == "not found");
    REQUIRE(doc["objuuid"] == "o1");
    REQUIRE(doc["coluuid"] == "c1");
}

TEST_CASE("GetConfig::from_json — request (config null)", "[control][get_config]")
{
    GetConfig f = GetConfig::from_json(GET_CONFIG_REQUEST_JSON);

    REQUIRE(!f.config_json.has_value());
    REQUIRE(!f.error.has_value());
    REQUIRE(!f.objuuid.has_value());
    REQUIRE(!f.coluuid.has_value());
}

TEST_CASE("GetConfig::from_json — response (config present)", "[control][get_config]")
{
    GetConfig f = GetConfig::from_json(GET_CONFIG_RESPONSE_JSON);

    REQUIRE(f.config_json.has_value());
    REQUIRE(!f.error.has_value());

    // Verify the embedded config JSON is well-formed and contains the
    // expected fields from the canonical Rust wire format.
    JsonDocument cfg = parse(*f.config_json);
    REQUIRE(cfg["agtuuid"] == "a1");
    REQUIRE(cfg["port"] == 8080);
}

TEST_CASE("GetConfig::from_json — error response", "[control][get_config]")
{
    GetConfig f = GetConfig::from_json(GET_CONFIG_ERROR_JSON);

    REQUIRE(!f.config_json.has_value());
    REQUIRE(f.error.has_value());
    REQUIRE(*f.error == "not found");
    REQUIRE(f.objuuid.has_value());
    REQUIRE(*f.objuuid == "o1");
    REQUIRE(f.coluuid.has_value());
    REQUIRE(*f.coluuid == "c1");
}

TEST_CASE("GetConfig roundtrip — request", "[control][get_config]")
{
    // Parse the canonical Rust JSON, re-serialise, and verify the output
    // retains all fields.
    GetConfig f = GetConfig::from_json(GET_CONFIG_REQUEST_JSON);
    JsonDocument doc = parse(f.to_json());

    REQUIRE(doc["type"] == "get_config");
    REQUIRE(doc["config"].isNull());
    REQUIRE(doc["error"].isNull());
}

TEST_CASE("GetConfig roundtrip — response", "[control][get_config]")
{
    GetConfig f = GetConfig::from_json(GET_CONFIG_RESPONSE_JSON);
    JsonDocument doc = parse(f.to_json());

    REQUIRE(doc["type"] == "get_config");
    REQUIRE(doc["config"]["agtuuid"] == "a1");
    REQUIRE(doc["config"]["port"] == 8080);
}

// ═════════════════════════════════════════════════════════════════════════════
// Benchmark
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("Benchmark::to_json — request (sizes null)", "[control][benchmark]")
{
    Benchmark f;
    JsonDocument doc = parse(f.to_json());

    REQUIRE(doc["type"] == "benchmark");
    REQUIRE(doc["outbound_size"].isNull());
    REQUIRE(doc["inbound_size"].isNull());
    REQUIRE(doc["payload"].isNull());
    REQUIRE(doc["error"].isNull());
    REQUIRE(doc["objuuid"].isNull());
    REQUIRE(doc["coluuid"].isNull());
}

TEST_CASE("Benchmark::to_json — request (outbound_size set)", "[control][benchmark]")
{
    Benchmark f;
    f.outbound_size = 1024;

    JsonDocument doc = parse(f.to_json());

    REQUIRE(doc["type"] == "benchmark");
    REQUIRE(doc["outbound_size"] == 1024);
    REQUIRE(doc["inbound_size"].isNull());
    REQUIRE(doc["payload"].isNull());
}

TEST_CASE("Benchmark::to_json — response (sizes and payload set)", "[control][benchmark]")
{
    Benchmark f;
    f.outbound_size = 1024;
    f.inbound_size = 512;
    f.payload = "aGVsbG8=";

    JsonDocument doc = parse(f.to_json());

    REQUIRE(doc["type"] == "benchmark");
    REQUIRE(doc["outbound_size"] == 1024);
    REQUIRE(doc["inbound_size"] == 512);
    REQUIRE(doc["payload"] == "aGVsbG8=");
    REQUIRE(doc["error"].isNull());
}

TEST_CASE("Benchmark::to_json — error response", "[control][benchmark]")
{
    Benchmark f;
    f.error = "timeout";
    f.objuuid = "o1";
    f.coluuid = "c1";

    JsonDocument doc = parse(f.to_json());

    REQUIRE(doc["type"] == "benchmark");
    REQUIRE(doc["outbound_size"].isNull());
    REQUIRE(doc["error"] == "timeout");
    REQUIRE(doc["objuuid"] == "o1");
    REQUIRE(doc["coluuid"] == "c1");
}

TEST_CASE("Benchmark::from_json — request", "[control][benchmark]")
{
    Benchmark f = Benchmark::from_json(BENCHMARK_REQUEST_JSON);

    REQUIRE(f.outbound_size.has_value());
    REQUIRE(*f.outbound_size == 1024);
    REQUIRE(!f.inbound_size.has_value());
    REQUIRE(!f.payload.has_value());
    REQUIRE(!f.error.has_value());
    REQUIRE(!f.objuuid.has_value());
    REQUIRE(!f.coluuid.has_value());
}

TEST_CASE("Benchmark::from_json — response", "[control][benchmark]")
{
    Benchmark f = Benchmark::from_json(BENCHMARK_RESPONSE_JSON);

    REQUIRE(f.outbound_size.has_value());
    REQUIRE(*f.outbound_size == 1024);
    REQUIRE(f.inbound_size.has_value());
    REQUIRE(*f.inbound_size == 512);
    REQUIRE(f.payload.has_value());
    REQUIRE(*f.payload == "aGVsbG8=");
    REQUIRE(!f.error.has_value());
}

TEST_CASE("Benchmark::from_json — error response", "[control][benchmark]")
{
    Benchmark f = Benchmark::from_json(BENCHMARK_ERROR_JSON);

    REQUIRE(!f.outbound_size.has_value());
    REQUIRE(!f.inbound_size.has_value());
    REQUIRE(f.error.has_value());
    REQUIRE(*f.error == "timeout");
    REQUIRE(f.objuuid.has_value());
    REQUIRE(*f.objuuid == "o1");
    REQUIRE(f.coluuid.has_value());
    REQUIRE(*f.coluuid == "c1");
}

TEST_CASE("Benchmark roundtrip — request", "[control][benchmark]")
{
    Benchmark f = Benchmark::from_json(BENCHMARK_REQUEST_JSON);
    JsonDocument doc = parse(f.to_json());

    REQUIRE(doc["type"] == "benchmark");
    REQUIRE(doc["outbound_size"] == 1024);
    REQUIRE(doc["inbound_size"].isNull());
    REQUIRE(doc["payload"].isNull());
}

TEST_CASE("Benchmark roundtrip — response", "[control][benchmark]")
{
    Benchmark f = Benchmark::from_json(BENCHMARK_RESPONSE_JSON);
    JsonDocument doc = parse(f.to_json());

    REQUIRE(doc["type"] == "benchmark");
    REQUIRE(doc["outbound_size"] == 1024);
    REQUIRE(doc["inbound_size"] == 512);
    REQUIRE(doc["payload"] == "aGVsbG8=");
}
