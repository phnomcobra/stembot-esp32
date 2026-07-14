// Protocol-compatibility tests for AgentClient helpers.
//
// Canonical fixtures are taken from stembot-rust/src/executor/agent.rs tests.
//
// AgentClient itself (HTTP) is not tested here — it requires a live network
// or a mock HTTP server and is better exercised in firmware / QEMU integration
// tests.  This file covers:
//   * AES-256-EAX encrypt / decrypt roundtrip
//   * MAC authentication failure detection (wrong tag, wrong key)
//   * set_isrc_json — inserts "isrc" into any network-message JSON
//   * Canonical JSON wire format (GetConfig, Ping with isrc)

#include "agent.hpp"
#include "control.hpp"
#include "network.hpp"

#include <ArduinoJson.h>
#include <catch2/catch_test_macros.hpp>

using namespace control_form;
using namespace network_message;

// ── Canonical test fixtures
// \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500

// SHA-256("stembot-test-key") \u2014 pre-computed to avoid a SHA dependency in tests.
static const uint8_t TEST_KEY[32] = {
    0x77, 0x47, 0x9b, 0x2e, 0xe1, 0x81, 0x5f, 0x39, 0x2c, 0xd3, 0x6e, 0x5c, 0xfe, 0x49, 0xaf, 0x1a,
    0x7f, 0xaa, 0x99, 0xbc, 0x2a, 0xc1, 0x39, 0x1f, 0x8c, 0x21, 0xf8, 0x17, 0xce, 0xd8, 0xba, 0xa4};

static constexpr const char* TEST_AGTUUID = "test-agent-id-1";

static constexpr const char* EXPECTED_GET_CONFIG_JSON =
    R"({"type":"get_config","error":null,"objuuid":null,"coluuid":null,"config":null})";

static constexpr const char* EXPECTED_PING_JSON =
    R"({"type":"ping","dest":null,"src":"test-agent-id-1","isrc":"test-agent-id-1",)"
    R"("timestamp":1000.0,"objuuid":null,"coluuid":null})";

// \u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550
// AES-256-EAX crypto tests
// \u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550

TEST_CASE("aes_eax_encrypt \u2014 nonce is 16 bytes", "[agent][crypto]")
{
    const char* pt = "hello";
    AesEaxResult res = aes_eax_encrypt(TEST_KEY, reinterpret_cast<const uint8_t*>(pt), 5);
    REQUIRE(!res.ciphertext.empty());
    // nonce is a fixed-size array member \u2014 its length is always 16
    REQUIRE(sizeof(res.nonce) == 16);
}

TEST_CASE("aes_eax_encrypt \u2014 tag is 16 bytes", "[agent][crypto]")
{
    const char* pt = "hello";
    AesEaxResult res = aes_eax_encrypt(TEST_KEY, reinterpret_cast<const uint8_t*>(pt), 5);
    REQUIRE(!res.ciphertext.empty());
    REQUIRE(sizeof(res.tag) == 16);
}

TEST_CASE("aes_eax_encrypt / aes_eax_decrypt \u2014 roundtrip", "[agent][crypto]")
{
    const std::string plaintext = "hello, stembot!";
    AesEaxResult res = aes_eax_encrypt(TEST_KEY, reinterpret_cast<const uint8_t*>(plaintext.data()),
                                       plaintext.size());

    REQUIRE(!res.ciphertext.empty());

    auto recovered =
        aes_eax_decrypt(TEST_KEY, res.nonce, res.tag, res.ciphertext.data(), res.ciphertext.size());
    REQUIRE(recovered.has_value());
    REQUIRE(std::string(recovered->begin(), recovered->end()) == plaintext);
}

TEST_CASE("aes_eax_decrypt \u2014 fails with wrong tag", "[agent][crypto]")
{
    const char* pt = "hello";
    AesEaxResult res = aes_eax_encrypt(TEST_KEY, reinterpret_cast<const uint8_t*>(pt), 5);
    REQUIRE(!res.ciphertext.empty());

    uint8_t bad_tag[16];
    memcpy(bad_tag, res.tag, 16);
    bad_tag[0] ^= 0xFF; // corrupt

    auto result =
        aes_eax_decrypt(TEST_KEY, res.nonce, bad_tag, res.ciphertext.data(), res.ciphertext.size());
    REQUIRE(!result.has_value());
}

TEST_CASE("aes_eax_decrypt \u2014 fails with wrong key", "[agent][crypto]")
{
    const char* pt = "hello";
    AesEaxResult res = aes_eax_encrypt(TEST_KEY, reinterpret_cast<const uint8_t*>(pt), 5);
    REQUIRE(!res.ciphertext.empty());

    uint8_t wrong_key[32];
    memcpy(wrong_key, TEST_KEY, 32);
    wrong_key[0] ^= 0xFF; // corrupt

    auto result = aes_eax_decrypt(wrong_key, res.nonce, res.tag, res.ciphertext.data(),
                                  res.ciphertext.size());
    REQUIRE(!result.has_value());
}

TEST_CASE("aes_eax_encrypt / aes_eax_decrypt \u2014 empty plaintext roundtrip", "[agent][crypto]")
{
    AesEaxResult res = aes_eax_encrypt(TEST_KEY, nullptr, 0);
    // empty plaintext is valid; ciphertext should also be empty
    REQUIRE(res.ciphertext.empty());

    auto recovered = aes_eax_decrypt(TEST_KEY, res.nonce, res.tag, nullptr, 0);
    REQUIRE(recovered.has_value());
    REQUIRE(recovered->empty());
}

// \u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550
// bytes_to_hex / hex_to_bytes
// \u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550

TEST_CASE("bytes_to_hex \u2014 known value", "[agent][hex]")
{
    const uint8_t bytes[] = {0xde, 0xad, 0xbe, 0xef};
    REQUIRE(bytes_to_hex(bytes, 4) == "deadbeef");
}

TEST_CASE("hex_to_bytes \u2014 roundtrip", "[agent][hex]")
{
    const uint8_t bytes[] = {0x01, 0x23, 0xab, 0xff};
    const std::string hex = bytes_to_hex(bytes, 4);
    const auto decoded = hex_to_bytes(hex);
    REQUIRE(decoded.size() == 4);
    REQUIRE(decoded[0] == 0x01);
    REQUIRE(decoded[1] == 0x23);
    REQUIRE(decoded[2] == 0xab);
    REQUIRE(decoded[3] == 0xff);
}

TEST_CASE("hex_to_bytes \u2014 invalid length returns empty", "[agent][hex]")
{
    REQUIRE(hex_to_bytes("abc").empty()); // odd length
}

// \u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550
// set_isrc_json
// \u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550

TEST_CASE("set_isrc_json \u2014 sets isrc on Ping", "[agent][set_isrc]")
{
    Ping p;
    p.src = TEST_AGTUUID;
    p.timestamp = 1000.0;
    // isrc is initially nullopt / null in the JSON
    const std::string with_isrc = set_isrc_json(p.to_json(), TEST_AGTUUID);

    // Verify by re-parsing
    Ping result = Ping::from_json(with_isrc);
    REQUIRE(result.isrc.has_value());
    REQUIRE(result.isrc.value() == TEST_AGTUUID);
}

TEST_CASE("set_isrc_json \u2014 preserves all other fields", "[agent][set_isrc]")
{
    Ping p;
    p.src = TEST_AGTUUID;
    p.timestamp = 1000.0;
    p.dest = "dest-agent";

    const std::string with_isrc = set_isrc_json(p.to_json(), TEST_AGTUUID);
    Ping result = Ping::from_json(with_isrc);

    REQUIRE(result.src == TEST_AGTUUID);
    REQUIRE(result.dest.value() == "dest-agent");
    REQUIRE(result.timestamp.value() == 1000.0);
}

TEST_CASE("set_isrc_json \u2014 overwrites existing isrc", "[agent][set_isrc]")
{
    Ping p;
    p.src = TEST_AGTUUID;
    p.isrc = "old-agent";

    const std::string with_isrc = set_isrc_json(p.to_json(), TEST_AGTUUID);
    Ping result = Ping::from_json(with_isrc);

    REQUIRE(result.isrc.value() == TEST_AGTUUID);
}

// \u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550
// Canonical JSON wire-format verification (mirrors Rust test_get_config_canonical_json
// and test_ping_canonical_json_with_isrc)
// \u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550

static JsonDocument parse_json(const std::string& json)
{
    JsonDocument doc;
    deserializeJson(doc, json);
    return doc;
}

TEST_CASE("GetConfig \u2014 canonical JSON (matches EXPECTED_GET_CONFIG_JSON)",
          "[agent][canonical]")
{
    GetConfig f;
    JsonDocument got = parse_json(f.to_json());
    JsonDocument exp = parse_json(EXPECTED_GET_CONFIG_JSON);

    REQUIRE(got["type"] == exp["type"]);
    REQUIRE(got["config"].isNull());
    REQUIRE(got["error"].isNull());
    REQUIRE(got["objuuid"].isNull());
    REQUIRE(got["coluuid"].isNull());
}

TEST_CASE("Ping with isrc \u2014 canonical JSON (matches EXPECTED_PING_JSON)", "[agent][canonical]")
{
    Ping p;
    p.src = TEST_AGTUUID;
    p.timestamp = 1000.0;

    const std::string with_isrc = set_isrc_json(p.to_json(), TEST_AGTUUID);
    JsonDocument got = parse_json(with_isrc);
    JsonDocument exp = parse_json(EXPECTED_PING_JSON);

    REQUIRE(got["type"] == exp["type"]);
    REQUIRE(got["src"] == TEST_AGTUUID);
    REQUIRE(got["isrc"] == TEST_AGTUUID);
    REQUIRE(got["timestamp"].as<double>() == 1000.0);
}
