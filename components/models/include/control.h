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

// ── Peer ──────────────────────────────────────────────────────────────────────
// Wire: { "agtuuid":null, "polling":false, "destroy_time":null,
//         "refresh_time":null, "url":null, "objuuid":null, "coluuid":null }

struct Peer
{
    std::optional<std::string> agtuuid;
    bool polling = false;
    std::optional<double> destroy_time;
    std::optional<double> refresh_time;
    std::optional<std::string> url;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string to_json() const;
    static Peer from_json(const std::string& json);
};

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

// ── GetPeers ──────────────────────────────────────────────────────────────────
// Wire type tag: "get_peers"

struct GetPeers
{
    std::vector<Peer> peers;
    std::optional<std::string> error;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string to_json() const;
    static GetPeers from_json(const std::string& json);
};

// ── Route ─────────────────────────────────────────────────────────────────────
// Wire: { "agtuuid":"", "gtwuuid":"", "weight":0, "objuuid":null, "coluuid":null }

struct Route
{
    std::string agtuuid;
    std::string gtwuuid;
    int64_t weight = 0;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string to_json() const;
    static Route from_json(const std::string& json);
};

// ── GetRoutes ─────────────────────────────────────────────────────────────────
// Wire type tag: "get_routes"

struct GetRoutes
{
    std::vector<Route> routes;
    std::optional<std::string> error;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string to_json() const;
    static GetRoutes from_json(const std::string& json);
};

// ── CreatePeer ────────────────────────────────────────────────────────────────
// Wire type tag: "create_peer"

struct CreatePeer
{
    std::string agtuuid;
    bool polling = false;
    std::optional<std::string> url;
    std::optional<double> ttl;
    std::optional<std::string> error;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string to_json() const;
    static CreatePeer from_json(const std::string& json);
};

// ── DiscoverPeer ──────────────────────────────────────────────────────────────
// Wire type tag: "discover_peer"

struct DiscoverPeer
{
    std::string url;
    bool polling = false;
    std::optional<std::string> agtuuid;
    std::optional<double> ttl;
    std::optional<std::string> error;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string to_json() const;
    static DiscoverPeer from_json(const std::string& json);
};

// ── DeletePeers ───────────────────────────────────────────────────────────────
// Wire type tag: "delete_peers"

struct DeletePeers
{
    std::vector<std::string> agtuuids; // empty means delete all
    std::optional<std::string> error;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string to_json() const;
    static DeletePeers from_json(const std::string& json);
};

// ── SyncProcess ───────────────────────────────────────────────────────────────
// Wire type tag: "sync_process"

struct SyncProcess
{
    std::string command;
    int64_t timeout = 15;
    std::optional<std::string> stdout_output;
    std::optional<std::string> stderr_output;
    std::optional<int64_t> status;
    std::optional<double> start_time;
    std::optional<double> elapsed_time;
    std::optional<std::string> error;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string to_json() const;
    static SyncProcess from_json(const std::string& json);
};

// ── WriteFile ─────────────────────────────────────────────────────────────────
// Wire type tag: "write_file"

struct WriteFile
{
    std::string b64zlib;
    std::string path;
    std::optional<int64_t> size;
    std::optional<std::string> md5sum;
    std::optional<std::string> error;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string to_json() const;
    static WriteFile from_json(const std::string& json);
};

// ── LoadFile ──────────────────────────────────────────────────────────────────
// Wire type tag: "load_file"

struct LoadFile
{
    std::string path;
    std::optional<std::string> b64zlib;
    std::optional<int64_t> size;
    std::optional<std::string> md5sum;
    std::optional<std::string> error;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string to_json() const;
    static LoadFile from_json(const std::string& json);
};
