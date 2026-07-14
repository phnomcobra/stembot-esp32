#include "control.hpp"

#include "utils.hpp"

#include <ArduinoJson.h>

namespace control_form
{

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
    if (strcmp(type, "benchmark") == 0)
        return ControlFormType::Benchmark;
    if (strcmp(type, "get_config") == 0)
        return ControlFormType::GetConfig;
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

} // namespace control_form
