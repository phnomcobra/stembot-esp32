#include "network.hpp"

#include "utils.hpp"

#include <ArduinoJson.h>
#include <algorithm>
#include <mbedtls/base64.h>

namespace network_message
{

namespace
{
const KeyDistribution* best_distribution(const std::vector<KeyDistribution>& distributions)
{
    if (distributions.empty())
        return nullptr;
    return &*std::max_element(distributions.begin(), distributions.end(),
                              [](const KeyDistribution& left, const KeyDistribution& right)
                              { return left.ttl < right.ttl; });
}
} // namespace

std::string ExchangeKeys::to_json() const
{
    JsonDocument doc;
    doc["type"] = "exchange_keys";
    set_opt_str(doc, "dest", dest);
    doc["src"] = src;
    set_opt_str(doc, "isrc", isrc);
    set_opt_f64(doc, "timestamp", timestamp);
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    JsonArray distributions_json = doc["distributions"].to<JsonArray>();
    for (const auto& distribution : distributions)
    {
        JsonObject item = distributions_json.add<JsonObject>();
        item["b64_key"] = distribution.b64_key;
        item["objuuid"] = distribution.objuuid;
        item["ttl"] = distribution.ttl;
    }
    std::string out;
    serializeJson(doc, out);
    return out;
}

ExchangeKeys ExchangeKeys::from_json(const std::string& json)
{
    ExchangeKeys message;
    JsonDocument doc;
    deserializeJson(doc, json);
    message.src = doc["src"] | std::string{};
    message.dest = get_opt_str(doc["dest"]);
    message.isrc = get_opt_str(doc["isrc"]);
    message.timestamp = get_opt_f64(doc["timestamp"]);
    message.objuuid = get_opt_str(doc["objuuid"]);
    message.coluuid = get_opt_str(doc["coluuid"]);
    for (JsonObjectConst item : doc["distributions"].as<JsonArrayConst>())
    {
        KeyDistribution distribution;
        distribution.b64_key = item["b64_key"] | std::string{};
        distribution.objuuid = item["objuuid"] | std::string{};
        distribution.ttl = item["ttl"] | 0.0;
        message.distributions.push_back(std::move(distribution));
    }
    return message;
}

std::optional<std::string> ExchangeKeys::best_key_uuid() const
{
    const KeyDistribution* distribution = best_distribution(distributions);
    if (distribution == nullptr)
        return std::nullopt;
    return distribution->objuuid;
}

std::optional<std::array<uint8_t, 32>> ExchangeKeys::best_key() const
{
    const KeyDistribution* distribution = best_distribution(distributions);
    if (distribution == nullptr)
        return std::nullopt;
    std::array<uint8_t, 32> key = {};
    size_t decoded_size = 0;
    if (mbedtls_base64_decode(key.data(), key.size(), &decoded_size,
                              reinterpret_cast<const unsigned char*>(distribution->b64_key.data()),
                              distribution->b64_key.size()) != 0 ||
        decoded_size != key.size())
        return std::nullopt;
    return key;
}

std::optional<double> ExchangeKeys::best_key_ttl() const
{
    const KeyDistribution* distribution = best_distribution(distributions);
    if (distribution == nullptr)
        return std::nullopt;
    return distribution->ttl;
}

// ── Ping ──────────────────────────────────────────────────────────────────────

std::string Ping::to_json() const
{
    JsonDocument doc;
    doc["type"] = "ping";
    set_opt_str(doc, "dest", dest);
    doc["src"] = src;
    set_opt_str(doc, "isrc", isrc);
    set_opt_f64(doc, "timestamp", timestamp);
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    std::string out;
    serializeJson(doc, out);
    return out;
}

Ping Ping::from_json(const std::string& json)
{
    Ping p;
    JsonDocument doc;
    deserializeJson(doc, json);
    p.src = doc["src"] | std::string{};
    p.dest = get_opt_str(doc["dest"]);
    p.isrc = get_opt_str(doc["isrc"]);
    p.timestamp = get_opt_f64(doc["timestamp"]);
    p.objuuid = get_opt_str(doc["objuuid"]);
    p.coluuid = get_opt_str(doc["coluuid"]);
    return p;
}

// ── NetworkMessagesRequest ────────────────────────────────────────────────────

std::string NetworkMessagesRequest::to_json() const
{
    JsonDocument doc;
    doc["type"] = "messages_request";
    set_opt_str(doc, "dest", dest);
    doc["src"] = src;
    set_opt_str(doc, "isrc", isrc);
    set_opt_f64(doc, "timestamp", timestamp);
    set_opt_i64(doc, "limit", limit);
    set_opt_str_array(doc, "network_whitelist", network_whitelist);
    set_opt_str_array(doc, "control_whitelist", control_whitelist);
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    std::string out;
    serializeJson(doc, out);
    return out;
}

NetworkMessagesRequest NetworkMessagesRequest::from_json(const std::string& json)
{
    NetworkMessagesRequest m;
    JsonDocument doc;
    deserializeJson(doc, json);
    m.src = doc["src"] | std::string{};
    m.dest = get_opt_str(doc["dest"]);
    m.isrc = get_opt_str(doc["isrc"]);
    m.timestamp = get_opt_f64(doc["timestamp"]);
    m.limit = get_opt_i64(doc["limit"]);
    m.network_whitelist = get_opt_str_array(doc["network_whitelist"]);
    m.control_whitelist = get_opt_str_array(doc["control_whitelist"]);
    m.objuuid = get_opt_str(doc["objuuid"]);
    m.coluuid = get_opt_str(doc["coluuid"]);
    return m;
}

// ── Acknowledgement ───────────────────────────────────────────────────────────

std::string Acknowledgement::to_json() const
{
    JsonDocument doc;
    doc["type"] = "acknowledgement";
    set_opt_str(doc, "dest", dest);
    doc["src"] = src;
    set_opt_str(doc, "isrc", isrc);
    set_opt_f64(doc, "timestamp", timestamp);
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    doc["ack_type"] = ack_type;
    set_opt_str(doc, "forwarded", forwarded);
    set_opt_str(doc, "error", error);
    std::string out;
    serializeJson(doc, out);
    return out;
}

Acknowledgement Acknowledgement::from_json(const std::string& json)
{
    Acknowledgement a;
    JsonDocument doc;
    deserializeJson(doc, json);
    a.ack_type = doc["ack_type"] | std::string{"ping"};
    a.src = doc["src"] | std::string{};
    a.dest = get_opt_str(doc["dest"]);
    a.isrc = get_opt_str(doc["isrc"]);
    a.timestamp = get_opt_f64(doc["timestamp"]);
    a.forwarded = get_opt_str(doc["forwarded"]);
    a.error = get_opt_str(doc["error"]);
    a.objuuid = get_opt_str(doc["objuuid"]);
    a.coluuid = get_opt_str(doc["coluuid"]);
    return a;
}

// ── NetworkMessagesResponse ───────────────────────────────────────────────────

std::string NetworkMessagesResponse::to_json() const
{
    JsonDocument doc;
    doc["type"] = "messages_response";
    set_opt_str(doc, "dest", dest);
    doc["src"] = src;
    set_opt_str(doc, "isrc", isrc);
    set_opt_f64(doc, "timestamp", timestamp);
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    JsonArray arr = doc["messages"].to<JsonArray>();
    for (const std::string& msg_json : message_jsons)
    {
        JsonDocument mdoc;
        deserializeJson(mdoc, msg_json);
        arr.add(mdoc.as<JsonVariant>());
    }
    std::string out;
    serializeJson(doc, out);
    return out;
}

NetworkMessagesResponse NetworkMessagesResponse::from_json(const std::string& json)
{
    NetworkMessagesResponse r;
    JsonDocument doc;
    deserializeJson(doc, json);
    r.src = doc["src"] | std::string{};
    r.dest = get_opt_str(doc["dest"]);
    r.isrc = get_opt_str(doc["isrc"]);
    r.timestamp = get_opt_f64(doc["timestamp"]);
    r.objuuid = get_opt_str(doc["objuuid"]);
    r.coluuid = get_opt_str(doc["coluuid"]);
    for (JsonVariant mv : doc["messages"].as<JsonArray>())
    {
        std::string mj;
        serializeJson(mv, mj);
        r.message_jsons.push_back(mj);
    }
    return r;
}

// ── TicketTraceResponse ───────────────────────────────────────────────────────

Hop TicketTraceResponse::hop() const
{
    return Hop{src, hop_time, network_ticket_type};
}

std::string TicketTraceResponse::to_json() const
{
    JsonDocument doc;
    doc["type"] = "ticket_trace_response";
    set_opt_str(doc, "dest", dest);
    doc["src"] = src;
    set_opt_str(doc, "isrc", isrc);
    set_opt_f64(doc, "timestamp", timestamp);
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    doc["tckuuid"] = tckuuid;
    doc["hop_time"] = hop_time;
    doc["network_ticket_type"] = network_ticket_type;
    std::string out;
    serializeJson(doc, out);
    return out;
}

TicketTraceResponse TicketTraceResponse::from_json(const std::string& json)
{
    TicketTraceResponse t;
    JsonDocument doc;
    deserializeJson(doc, json);
    t.tckuuid = doc["tckuuid"] | std::string{};
    t.network_ticket_type = doc["network_ticket_type"] | std::string{};
    t.hop_time = doc["hop_time"] | 0.0;
    t.src = doc["src"] | std::string{};
    t.dest = get_opt_str(doc["dest"]);
    t.isrc = get_opt_str(doc["isrc"]);
    t.timestamp = get_opt_f64(doc["timestamp"]);
    t.objuuid = get_opt_str(doc["objuuid"]);
    t.coluuid = get_opt_str(doc["coluuid"]);
    return t;
}

// ── NetworkTicket ─────────────────────────────────────────────────────────────

std::string NetworkTicket::to_json(const std::string& type_tag) const
{
    JsonDocument doc;
    doc["type"] = type_tag;
    set_opt_str(doc, "dest", dest);
    doc["src"] = src;
    set_opt_str(doc, "isrc", isrc);
    set_opt_f64(doc, "timestamp", timestamp);
    set_opt_str(doc, "objuuid", objuuid);
    set_opt_str(doc, "coluuid", coluuid);
    doc["tckuuid"] = tckuuid;
    set_opt_str(doc, "error", error);
    set_opt_f64(doc, "create_time", create_time);
    set_opt_f64(doc, "service_time", service_time);
    doc["tracing"] = tracing;
    if (!form_json.empty())
    {
        JsonDocument fdoc;
        deserializeJson(fdoc, form_json);
        doc["form"] = fdoc;
    }
    else
    {
        doc["form"] = nullptr;
    }
    std::string out;
    serializeJson(doc, out);
    return out;
}

NetworkTicket NetworkTicket::from_json(const std::string& json)
{
    NetworkTicket t;
    JsonDocument doc;
    deserializeJson(doc, json);
    t.tckuuid = doc["tckuuid"] | std::string{};
    t.tracing = doc["tracing"] | false;
    t.src = doc["src"] | std::string{};
    t.dest = get_opt_str(doc["dest"]);
    t.isrc = get_opt_str(doc["isrc"]);
    t.timestamp = get_opt_f64(doc["timestamp"]);
    t.create_time = get_opt_f64(doc["create_time"]);
    t.service_time = get_opt_f64(doc["service_time"]);
    t.error = get_opt_str(doc["error"]);
    t.objuuid = get_opt_str(doc["objuuid"]);
    t.coluuid = get_opt_str(doc["coluuid"]);
    if (!doc["form"].isNull())
    {
        serializeJson(doc["form"], t.form_json);
    }
    return t;
}

// ── network_message_type ──────────────────────────────────────────────────────

NetworkMessageType network_message_type(const std::string& json)
{
    JsonDocument doc;
    deserializeJson(doc, json);
    const char* type = doc["type"] | "";
    if (strcmp(type, "ping") == 0)
        return NetworkMessageType::Ping;
    if (strcmp(type, "exchange_keys") == 0)
        return NetworkMessageType::ExchangeKeys;
    if (strcmp(type, "messages_request") == 0)
        return NetworkMessageType::MessagesRequest;
    if (strcmp(type, "messages_response") == 0)
        return NetworkMessageType::MessagesResponse;
    if (strcmp(type, "acknowledgement") == 0)
        return NetworkMessageType::Acknowledgement;
    if (strcmp(type, "ticket_trace_response") == 0)
        return NetworkMessageType::TicketTraceResponse;
    if (strcmp(type, "ticket_request") == 0)
        return NetworkMessageType::TicketRequest;
    if (strcmp(type, "ticket_response") == 0)
        return NetworkMessageType::TicketResponse;
    return NetworkMessageType::Unknown;
}

} // namespace network_message
