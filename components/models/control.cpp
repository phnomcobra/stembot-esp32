#include "control.hpp"

#include "utils.hpp"

#include <ArduinoJson.h>

// ── Peer ──────────────────────────────────────────────────────────────────────

std::string Peer::to_json() const
{
    JsonDocument doc;
    set_opt_str(doc, "agtuuid", agtuuid);
    doc["polling"] = polling;
    set_opt_f64(doc, "destroy_time", destroy_time);
    set_opt_f64(doc, "refresh_time", refresh_time);
    set_opt_str(doc, "url", url);
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    std::string out;
    serializeJson(doc, out);
    return out;
}

Peer Peer::from_json(const std::string& json)
{
    Peer p;
    JsonDocument doc;
    deserializeJson(doc, json);
    p.agtuuid = get_opt_str(doc["agtuuid"]);
    p.polling = doc["polling"] | false;
    p.destroy_time = get_opt_f64(doc["destroy_time"]);
    p.refresh_time = get_opt_f64(doc["refresh_time"]);
    p.url = get_opt_str(doc["url"]);
    p.objuuid = get_opt_str(doc["objuuid"]);
    p.coluuid = get_opt_str(doc["coluuid"]);
    return p;
}

// ── GetConfig ─────────────────────────────────────────────────────────────────

std::string GetConfig::to_json() const
{
    JsonDocument doc;
    doc["type"] = "get_config";
    if (config_json)
    {
        doc["config"] = serialized(*config_json);
    }
    else
    {
        doc["config"] = nullptr;
    }
    set_opt_str(doc, "error", error);
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    std::string out;
    serializeJson(doc, out);
    return out;
}

GetConfig GetConfig::from_json(const std::string& json)
{
    GetConfig f;
    JsonDocument doc;
    deserializeJson(doc, json);
    if (!doc["config"].isNull())
    {
        std::string raw;
        serializeJson(doc["config"], raw);
        f.config_json = raw;
    }
    f.error = get_opt_str(doc["error"]);
    f.objuuid = get_opt_str(doc["objuuid"]);
    f.coluuid = get_opt_str(doc["coluuid"]);
    return f;
}
// ── control_form_type ─────────────────────────────────────────────────────────

ControlFormType control_form_type(const std::string& json)
{
    JsonDocument doc;
    deserializeJson(doc, json);
    const char* type = doc["type"] | "";
    if (strcmp(type, "create_peer") == 0)
        return ControlFormType::CreatePeer;
    if (strcmp(type, "discover_peer") == 0)
        return ControlFormType::DiscoverPeer;
    if (strcmp(type, "delete_peers") == 0)
        return ControlFormType::DeletePeers;
    if (strcmp(type, "get_peers") == 0)
        return ControlFormType::GetPeers;
    if (strcmp(type, "get_routes") == 0)
        return ControlFormType::GetRoutes;
    if (strcmp(type, "sync_process") == 0)
        return ControlFormType::SyncProcess;
    if (strcmp(type, "write_file") == 0)
        return ControlFormType::WriteFile;
    if (strcmp(type, "load_file") == 0)
        return ControlFormType::LoadFile;
    if (strcmp(type, "benchmark") == 0)
        return ControlFormType::Benchmark;
    if (strcmp(type, "get_config") == 0)
        return ControlFormType::GetConfig;
    if (strcmp(type, "check_ticket") == 0)
        return ControlFormType::CheckTicket;
    if (strcmp(type, "close_ticket") == 0)
        return ControlFormType::CloseTicket;
    return ControlFormType::Unknown;
}
// ── Benchmark ─────────────────────────────────────────────────────────────────

std::string Benchmark::to_json() const
{
    JsonDocument doc;
    doc["type"] = "benchmark";
    set_opt_i64(doc, "outbound_size", outbound_size);
    set_opt_i64(doc, "inbound_size", inbound_size);
    set_opt_str(doc, "payload", payload);
    set_opt_str(doc, "error", error);
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    std::string out;
    serializeJson(doc, out);
    return out;
}

Benchmark Benchmark::from_json(const std::string& json)
{
    Benchmark f;
    JsonDocument doc;
    deserializeJson(doc, json);
    f.outbound_size = get_opt_i64(doc["outbound_size"]);
    f.inbound_size = get_opt_i64(doc["inbound_size"]);
    f.payload = get_opt_str(doc["payload"]);
    f.error = get_opt_str(doc["error"]);
    f.objuuid = get_opt_str(doc["objuuid"]);
    f.coluuid = get_opt_str(doc["coluuid"]);
    return f;
}

// ── GetPeers ──────────────────────────────────────────────────────────────────

std::string GetPeers::to_json() const
{
    JsonDocument doc;
    doc["type"] = "get_peers";
    JsonArray arr = doc["peers"].to<JsonArray>();
    for (const Peer& peer : peers)
    {
        JsonDocument peer_doc;
        deserializeJson(peer_doc, peer.to_json());
        arr.add(peer_doc.as<JsonObject>());
    }
    set_opt_str(doc, "error", error);
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    std::string out;
    serializeJson(doc, out);
    return out;
}

GetPeers GetPeers::from_json(const std::string& json)
{
    GetPeers f;
    JsonDocument doc;
    deserializeJson(doc, json);
    for (JsonVariant peer_var : doc["peers"].as<JsonArray>())
    {
        std::string peer_json;
        serializeJson(peer_var, peer_json);
        f.peers.push_back(Peer::from_json(peer_json));
    }
    f.error = get_opt_str(doc["error"]);
    f.objuuid = get_opt_str(doc["objuuid"]);
    f.coluuid = get_opt_str(doc["coluuid"]);
    return f;
}

// ── Route ─────────────────────────────────────────────────────────────────────

std::string Route::to_json() const
{
    JsonDocument doc;
    doc["agtuuid"] = agtuuid;
    doc["gtwuuid"] = gtwuuid;
    doc["weight"] = weight;
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    std::string out;
    serializeJson(doc, out);
    return out;
}

Route Route::from_json(const std::string& json)
{
    Route r;
    JsonDocument doc;
    deserializeJson(doc, json);
    r.agtuuid = doc["agtuuid"] | std::string{};
    r.gtwuuid = doc["gtwuuid"] | std::string{};
    r.weight = doc["weight"] | int64_t{0};
    r.objuuid = get_opt_str(doc["objuuid"]);
    r.coluuid = get_opt_str(doc["coluuid"]);
    return r;
}

// ── GetRoutes ─────────────────────────────────────────────────────────────────

std::string GetRoutes::to_json() const
{
    JsonDocument doc;
    doc["type"] = "get_routes";
    JsonArray arr = doc["routes"].to<JsonArray>();
    for (const Route& route : routes)
    {
        JsonDocument rdoc;
        deserializeJson(rdoc, route.to_json());
        arr.add(rdoc.as<JsonObject>());
    }
    set_opt_str(doc, "error", error);
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    std::string out;
    serializeJson(doc, out);
    return out;
}

GetRoutes GetRoutes::from_json(const std::string& json)
{
    GetRoutes f;
    JsonDocument doc;
    deserializeJson(doc, json);
    for (JsonVariant rv : doc["routes"].as<JsonArray>())
    {
        std::string rj;
        serializeJson(rv, rj);
        f.routes.push_back(Route::from_json(rj));
    }
    f.error = get_opt_str(doc["error"]);
    f.objuuid = get_opt_str(doc["objuuid"]);
    f.coluuid = get_opt_str(doc["coluuid"]);
    return f;
}

// ── CreatePeer ────────────────────────────────────────────────────────────────

std::string CreatePeer::to_json() const
{
    JsonDocument doc;
    doc["type"] = "create_peer";
    doc["agtuuid"] = agtuuid;
    doc["polling"] = polling;
    set_opt_str(doc, "url", url);
    set_opt_f64(doc, "ttl", ttl);
    set_opt_str(doc, "error", error);
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    std::string out;
    serializeJson(doc, out);
    return out;
}

CreatePeer CreatePeer::from_json(const std::string& json)
{
    CreatePeer f;
    JsonDocument doc;
    deserializeJson(doc, json);
    f.agtuuid = doc["agtuuid"] | std::string{};
    f.polling = doc["polling"] | false;
    f.url = get_opt_str(doc["url"]);
    f.ttl = get_opt_f64(doc["ttl"]);
    f.error = get_opt_str(doc["error"]);
    f.objuuid = get_opt_str(doc["objuuid"]);
    f.coluuid = get_opt_str(doc["coluuid"]);
    return f;
}

// ── DiscoverPeer ──────────────────────────────────────────────────────────────

std::string DiscoverPeer::to_json() const
{
    JsonDocument doc;
    doc["type"] = "discover_peer";
    doc["url"] = url;
    doc["polling"] = polling;
    set_opt_str(doc, "agtuuid", agtuuid);
    set_opt_f64(doc, "ttl", ttl);
    set_opt_str(doc, "error", error);
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    std::string out;
    serializeJson(doc, out);
    return out;
}

DiscoverPeer DiscoverPeer::from_json(const std::string& json)
{
    DiscoverPeer f;
    JsonDocument doc;
    deserializeJson(doc, json);
    f.url = doc["url"] | std::string{};
    f.polling = doc["polling"] | false;
    f.agtuuid = get_opt_str(doc["agtuuid"]);
    f.ttl = get_opt_f64(doc["ttl"]);
    f.error = get_opt_str(doc["error"]);
    f.objuuid = get_opt_str(doc["objuuid"]);
    f.coluuid = get_opt_str(doc["coluuid"]);
    return f;
}

// ── DeletePeers ───────────────────────────────────────────────────────────────

std::string DeletePeers::to_json() const
{
    JsonDocument doc;
    doc["type"] = "delete_peers";
    if (agtuuids.empty())
    {
        doc["agtuuids"] = nullptr;
    }
    else
    {
        JsonArray arr = doc["agtuuids"].to<JsonArray>();
        for (const std::string& id : agtuuids)
        {
            arr.add(id);
        }
    }
    set_opt_str(doc, "error", error);
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    std::string out;
    serializeJson(doc, out);
    return out;
}

DeletePeers DeletePeers::from_json(const std::string& json)
{
    DeletePeers f;
    JsonDocument doc;
    deserializeJson(doc, json);
    if (!doc["agtuuids"].isNull())
    {
        for (JsonVariant v : doc["agtuuids"].as<JsonArray>())
        {
            f.agtuuids.push_back(v.as<std::string>());
        }
    }
    f.error = get_opt_str(doc["error"]);
    f.objuuid = get_opt_str(doc["objuuid"]);
    f.coluuid = get_opt_str(doc["coluuid"]);
    return f;
}

// ── SyncProcess ───────────────────────────────────────────────────────────────

std::string SyncProcess::to_json() const
{
    JsonDocument doc;
    doc["type"] = "sync_process";
    doc["command"] = command;
    doc["timeout"] = timeout;
    set_opt_str(doc, "stdout", stdout_output);
    set_opt_str(doc, "stderr", stderr_output);
    set_opt_i64(doc, "status", status);
    set_opt_f64(doc, "start_time", start_time);
    set_opt_f64(doc, "elapsed_time", elapsed_time);
    set_opt_str(doc, "error", error);
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    std::string out;
    serializeJson(doc, out);
    return out;
}

SyncProcess SyncProcess::from_json(const std::string& json)
{
    SyncProcess f;
    JsonDocument doc;
    deserializeJson(doc, json);
    f.command = doc["command"] | std::string{};
    f.timeout = doc["timeout"] | int64_t{15};
    f.stdout_output = get_opt_str(doc["stdout"]);
    f.stderr_output = get_opt_str(doc["stderr"]);
    f.status = get_opt_i64(doc["status"]);
    f.start_time = get_opt_f64(doc["start_time"]);
    f.elapsed_time = get_opt_f64(doc["elapsed_time"]);
    f.error = get_opt_str(doc["error"]);
    f.objuuid = get_opt_str(doc["objuuid"]);
    f.coluuid = get_opt_str(doc["coluuid"]);
    return f;
}

// ── WriteFile ─────────────────────────────────────────────────────────────────

std::string WriteFile::to_json() const
{
    JsonDocument doc;
    doc["type"] = "write_file";
    doc["b64zlib"] = b64zlib;
    doc["path"] = path;
    set_opt_i64(doc, "size", size);
    set_opt_str(doc, "md5sum", md5sum);
    set_opt_str(doc, "error", error);
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    std::string out;
    serializeJson(doc, out);
    return out;
}

WriteFile WriteFile::from_json(const std::string& json)
{
    WriteFile f;
    JsonDocument doc;
    deserializeJson(doc, json);
    f.b64zlib = doc["b64zlib"] | std::string{};
    f.path = doc["path"] | std::string{};
    f.size = get_opt_i64(doc["size"]);
    f.md5sum = get_opt_str(doc["md5sum"]);
    f.error = get_opt_str(doc["error"]);
    f.objuuid = get_opt_str(doc["objuuid"]);
    f.coluuid = get_opt_str(doc["coluuid"]);
    return f;
}

// ── LoadFile ──────────────────────────────────────────────────────────────────

std::string LoadFile::to_json() const
{
    JsonDocument doc;
    doc["type"] = "load_file";
    doc["path"] = path;
    set_opt_str(doc, "b64zlib", b64zlib);
    set_opt_i64(doc, "size", size);
    set_opt_str(doc, "md5sum", md5sum);
    set_opt_str(doc, "error", error);
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    std::string out;
    serializeJson(doc, out);
    return out;
}

LoadFile LoadFile::from_json(const std::string& json)
{
    LoadFile f;
    JsonDocument doc;
    deserializeJson(doc, json);
    f.path = doc["path"] | std::string{};
    f.b64zlib = get_opt_str(doc["b64zlib"]);
    f.size = get_opt_i64(doc["size"]);
    f.md5sum = get_opt_str(doc["md5sum"]);
    f.error = get_opt_str(doc["error"]);
    f.objuuid = get_opt_str(doc["objuuid"]);
    f.coluuid = get_opt_str(doc["coluuid"]);
    return f;
}
