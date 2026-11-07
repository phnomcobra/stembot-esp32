#include "control.hpp"

#include "utils.hpp"

#include <ArduinoJson.h>

namespace control_form
{

std::string GetKeyMetadata::to_json() const
{
    JsonDocument doc;
    doc["type"] = "get_key_metadata";
    set_opt_str(doc, "error", error);
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    if (metadata)
    {
        JsonArray array = doc["metadata"].to<JsonArray>();
        for (const auto& item : *metadata)
        {
            JsonObject entry = array.add<JsonObject>();
            entry["objuuid"] = item.objuuid;
            entry["type"] = key_type_to_string(item.type);
            if (item.agtuuid)
                entry["agtuuid"] = *item.agtuuid;
            else
                entry["agtuuid"] = nullptr;
            entry["ttl"] = item.ttl;
        }
    }
    else
    {
        doc["metadata"] = nullptr;
    }
    std::string out;
    serializeJson(doc, out);
    return out;
}

GetKeyMetadata GetKeyMetadata::from_json(const std::string& json)
{
    GetKeyMetadata form;
    JsonDocument doc;
    deserializeJson(doc, json);
    form.error = get_opt_str(doc["error"]);
    form.objuuid = get_opt_str(doc["objuuid"]);
    form.coluuid = get_opt_str(doc["coluuid"]);
    if (!doc["metadata"].isNull())
    {
        form.metadata = std::vector<KeyMetadata>{};
        for (JsonObjectConst item : doc["metadata"].as<JsonArrayConst>())
        {
            KeyMetadata metadata;
            metadata.objuuid = item["objuuid"] | std::string{};
            metadata.type = key_type_from_string(item["type"] | std::string{"unknown"});
            metadata.agtuuid = get_opt_str(item["agtuuid"]);
            metadata.ttl = item["ttl"] | 0.0;
            form.metadata->push_back(std::move(metadata));
        }
    }
    return form;
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
    if (strcmp(type, "benchmark") == 0)
        return ControlFormType::Benchmark;
    if (strcmp(type, "get_config") == 0)
        return ControlFormType::GetConfig;
    if (strcmp(type, "get_key_metadata") == 0)
        return ControlFormType::GetKeyMetadata;
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
