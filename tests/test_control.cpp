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
#include "crypto.hpp"

#include <ArduinoJson.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <ctime>

using namespace control_form;

TEST_CASE("key type parsing is case-insensitive", "[crypto][key_type]")
{
    REQUIRE(key_type_from_string("user") == KeyType::USER);
    REQUIRE(key_type_from_string("USER") == KeyType::USER);
    REQUIRE(key_type_from_string("UsEr") == KeyType::USER);
    REQUIRE(key_type_from_string("dist") == KeyType::DIST);
    REQUIRE(key_type_from_string("DIST") == KeyType::DIST);
    REQUIRE(key_type_from_string("DiSt") == KeyType::DIST);
    REQUIRE(key_type_from_string("other") == KeyType::UNKNOWN);
}

TEST_CASE("user key expiration is ten minutes from now", "[crypto][user_key]")
{
    const double now = static_cast<double>(std::time(nullptr));
    const double expiration = user_key_expire_time();
    REQUIRE(expiration >= now + 600.0);
    REQUIRE(expiration <= now + 601.0);
}

TEST_CASE("user key derivation matches Python and Rust v1", "[crypto][user_key]")
{
    uint8_t key[32] = {};
    REQUIRE(derive_user_key(key, "correct horse battery staple", "peer-mesh-01"));

    constexpr uint8_t expected[32] = {0x69, 0x8a, 0x94, 0xe2, 0xfd, 0xec, 0x52, 0x00,
                                      0x11, 0x3e, 0x6e, 0xd8, 0xd8, 0x9e, 0x8a, 0x7b,
                                      0x9a, 0x4e, 0xf6, 0x04, 0x1e, 0x0d, 0x0e, 0x06,
                                      0x3d, 0x87, 0xe9, 0xe7, 0x10, 0x8c, 0xce, 0xe5};
    REQUIRE(std::memcmp(key, expected, sizeof(expected)) == 0);
}

static JsonDocument parse(const std::string& json)
{
    JsonDocument doc;
    auto err = deserializeJson(doc, json);
    REQUIRE(!err);
    return doc;
}

TEST_CASE("GetKeyMetadata matches the v3 wire format", "[control][get_key_metadata]")
{
    GetKeyMetadata form;
    JsonDocument request = parse(form.to_json());
    REQUIRE(request["type"] == "get_key_metadata");
    REQUIRE(request["metadata"].isNull());

    form.metadata =
        std::vector<KeyMetadata>{{"key-1", KeyType::DIST, std::string{"peer-1"}, 120.5}};
    JsonDocument response = parse(form.to_json());
    REQUIRE(response["metadata"][0]["objuuid"] == "key-1");
    REQUIRE(response["metadata"][0]["type"] == "dist");
    REQUIRE(response["metadata"][0]["agtuuid"] == "peer-1");
    REQUIRE(response["metadata"][0]["ttl"].as<double>() == Catch::Approx(120.5));

    const auto parsed = GetKeyMetadata::from_json(
        R"({"type":"get_key_metadata","error":null,"objuuid":null,"coluuid":null,)"
        R"("metadata":[{"objuuid":"key-1","type":"user","agtuuid":null,"ttl":0}])");
    REQUIRE(parsed.metadata.has_value());
    REQUIRE(parsed.metadata->front().type == KeyType::USER);
    REQUIRE(!parsed.metadata->front().agtuuid.has_value());
}

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
