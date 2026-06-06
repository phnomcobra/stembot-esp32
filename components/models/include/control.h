#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Control forms — port of stembot-rust/src/models/control.rs (GetConfig,
// Benchmark, GetPeers only).  Serialization and deserialization use only the
// C++ standard library; no external JSON dependency is required.
//
// All optional fields serialise as JSON null to match the Python/Rust wire
// format.  The type tag is embedded in each object's JSON output.

// ── Peer ──────────────────────────────────────────────────────────────────────
// Wire: { "agtuuid":null, "polling":false, "destroy_time":null,
//         "refresh_time":null, "url":null, "objuuid":null, "coluuid":null }

struct Peer {
    std::optional<std::string> agtuuid;
    bool                       polling      = false;
    std::optional<double>      destroy_time;
    std::optional<double>      refresh_time;
    std::optional<std::string> url;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string      to_json() const;
    static Peer      from_json(const std::string &json);
};

// ── GetConfig ─────────────────────────────────────────────────────────────────
// Wire type tag: "get_config"
// config_json: raw JSON string of the config object, or nullopt for null.

struct GetConfig {
    std::optional<std::string> config_json;
    std::optional<std::string> error;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string         to_json() const;
    static GetConfig    from_json(const std::string &json);
};

// ── Benchmark ─────────────────────────────────────────────────────────────────
// Wire type tag: "benchmark"

struct Benchmark {
    std::optional<int64_t>     outbound_size;
    std::optional<int64_t>     inbound_size;
    std::optional<std::string> payload;
    std::optional<std::string> error;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string         to_json() const;
    static Benchmark    from_json(const std::string &json);
};

// ── GetPeers ──────────────────────────────────────────────────────────────────
// Wire type tag: "get_peers"

struct GetPeers {
    std::vector<Peer>          peers;
    std::optional<std::string> error;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string         to_json() const;
    static GetPeers     from_json(const std::string &json);
};

