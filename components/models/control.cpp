#include "control.h"

#include "utils.h"

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
