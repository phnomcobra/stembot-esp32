#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Control forms — port of stembot-rust/src/models/control.rs (GetConfig,
// Benchmark, GetPeers only).  Serialization uses ArduinoJson.
//
// All optional fields serialise as JSON null to match the Python/Rust wire
// format.  The type tag is embedded in each object's JSON output.

// ── GetConfig ─────────────────────────────────────────────────────────────────
// Wire type tag: "get_config"
// config_json: raw JSON string of the config object, or nullopt for null.

struct GetConfig
{
    std::optional<std::string> config_json;
    std::optional<std::string> error;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string to_json() const;
    static GetConfig from_json(const std::string& json);
};

// ── Benchmark ─────────────────────────────────────────────────────────────────
// Wire type tag: "benchmark"

struct Benchmark
{
    std::optional<int64_t> outbound_size;
    std::optional<int64_t> inbound_size;
    std::optional<std::string> payload;
    std::optional<std::string> error;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string to_json() const;
    static Benchmark from_json(const std::string& json);
};

// ── ControlFormType ───────────────────────────────────────────────────────────
// Discriminant for the ControlForm tagged union.
// Mirrors the ControlForm enum variants in stembot-rust/src/models/control.rs.

enum class ControlFormType
{
    Benchmark,
    GetConfig,
    Unknown
};

// Read the "type" field of a raw JSON string and return the corresponding
// ControlFormType.  Returns Unknown if the type tag is absent or
// unrecognised.  Does not fully deserialise the form.
ControlFormType control_form_type(const std::string& json);
