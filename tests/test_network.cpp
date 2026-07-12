// Protocol-compatibility tests for the network message models.
//
// Canonical JSON strings are taken directly from the Rust reference
// implementation (stembot-rust/src/models/network.rs) to enforce wire
// compatibility.  Field ordering within an object is intentionally
// irrelevant — each test parses the serialised output with ArduinoJson and
// inspects individual fields, matching the order-insensitive strategy used
// by the Rust assert_ser_eq / assert_deser_roundtrip helpers.

#include "network.hpp"

#include <ArduinoJson.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

// ── canonical wire JSON from stembot-rust/src/models/network.rs ──────────────

static constexpr const char* PING_JSON =
    R"({"type":"ping","dest":null,"src":"a1","isrc":null,"timestamp":1000.0,)"
    R"("objuuid":null,"coluuid":null})";

static constexpr const char* MSGS_REQUEST_JSON =
    R"({"type":"messages_request","dest":null,"src":"a1","isrc":null,"timestamp":1000.0,)"
    R"("limit":null,"objuuid":null,"coluuid":null})";

static constexpr const char* ACK_PING_JSON =
    R"({"type":"acknowledgement","dest":null,"src":"a1","isrc":null,"timestamp":1000.0,)"
    R"("objuuid":null,"coluuid":null,"ack_type":"ping","forwarded":null,"error":null})";

static constexpr const char* ACK_ERROR_JSON =
    R"({"type":"acknowledgement","dest":null,"src":"a1","isrc":null,"timestamp":1000.0,)"
    R"("objuuid":null,"coluuid":null,"ack_type":"ticket_request","forwarded":null,"error":"timeout"})";

static constexpr const char* ACK_FORWARDED_JSON =
    R"({"type":"acknowledgement","dest":null,"src":"a1","isrc":null,"timestamp":1000.0,)"
    R"("objuuid":null,"coluuid":null,"ack_type":"ping","forwarded":"a2","error":null})";

static constexpr const char* ADV_ROUTES_JSON =
    R"({"type":"advertisement","dest":null,"src":"a1","isrc":null,"timestamp":1000.0,)"
    R"("objuuid":null,"coluuid":null,)"
    R"("routes":[{"agtuuid":"a2","gtwuuid":"a1","weight":1,"objuuid":null,"coluuid":null}],)"
    R"("agtuuid":"a1"})";

static constexpr const char* MSGS_RESP_EMPTY_JSON =
    R"({"type":"messages_response","dest":null,"src":"a1","isrc":null,"timestamp":1000.0,)"
    R"("objuuid":null,"coluuid":null,"messages":[]})";

static constexpr const char* MSGS_RESP_WITH_PING_JSON =
    R"({"type":"messages_response","dest":null,"src":"a1","isrc":null,"timestamp":1000.0,)"
    R"("objuuid":null,"coluuid":null,)"
    R"("messages":[{"type":"ping","dest":null,"src":"b1","isrc":null,"timestamp":2000.0,)"
    R"("objuuid":null,"coluuid":null}]})";

static constexpr const char* TTR_JSON =
    R"({"type":"ticket_trace_response","dest":null,"src":"a1","isrc":null,"timestamp":1000.0,)"
    R"("objuuid":null,"coluuid":null,"tckuuid":"t1","hop_time":1000.0,)"
    R"("network_ticket_type":"ticket_request"})";

// SyncProcess form embedded in a ticket — command: "ls /", timeout: 15, no output yet.
static constexpr const char* NT_REQUEST_JSON =
    R"({"type":"ticket_request","dest":null,"src":"a1","isrc":null,"timestamp":1000.0,)"
    R"("objuuid":null,"coluuid":null,"tckuuid":"t1","error":null,"create_time":null,)"
    R"("service_time":null,"tracing":false,)"
    R"("form":{"type":"sync_process","error":null,"objuuid":null,"coluuid":null,)"
    R"("timeout":15,"command":"ls /","stdout":null,"stderr":null,)"
    R"("status":null,"start_time":null,"elapsed_time":null}})";

// Same ticket after execution: stdout filled in, service_time recorded.
static constexpr const char* NT_RESPONSE_JSON =
    R"({"type":"ticket_response","dest":null,"src":"a1","isrc":null,"timestamp":1000.0,)"
    R"("objuuid":null,"coluuid":null,"tckuuid":"t1","error":null,"create_time":null,)"
    R"("service_time":0.5,"tracing":false,)"
    R"("form":{"type":"sync_process","error":null,"objuuid":null,"coluuid":null,)"
    R"("timeout":15,"command":"ls /","stdout":"bin\n","stderr":null,)"
    R"("status":0,"start_time":1000.0,"elapsed_time":0.1}})";

// ── helpers ───────────────────────────────────────────────────────────────────

static JsonDocument parse(const std::string& json)
{
    JsonDocument doc;
    auto err = deserializeJson(doc, json);
    REQUIRE(!err);
    return doc;
}

// ═════════════════════════════════════════════════════════════════════════════
// Ping
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("Ping::to_json — basic", "[network][ping]")
{
    Ping p;
    p.src = "a1";
    p.timestamp = 1000.0;

    JsonDocument doc = parse(p.to_json());
    REQUIRE(doc["type"] == "ping");
    REQUIRE(doc["src"] == "a1");
    REQUIRE(doc["timestamp"].as<double>() == Catch::Approx(1000.0));
    REQUIRE(doc["dest"].isNull());
    REQUIRE(doc["isrc"].isNull());
    REQUIRE(doc["objuuid"].isNull());
    REQUIRE(doc["coluuid"].isNull());
}

TEST_CASE("Ping::from_json — canonical roundtrip", "[network][ping]")
{
    Ping p = Ping::from_json(PING_JSON);
    JsonDocument doc = parse(p.to_json());
    REQUIRE(doc["type"] == "ping");
    REQUIRE(doc["src"] == "a1");
    REQUIRE(doc["timestamp"].as<double>() == Catch::Approx(1000.0));
    REQUIRE(doc["dest"].isNull());
}

TEST_CASE("network_message_type — ping", "[network][type_detection]")
{
    REQUIRE(network_message_type(PING_JSON) == NetworkMessageType::Ping);
}

// ═════════════════════════════════════════════════════════════════════════════
// NetworkMessagesRequest
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("NetworkMessagesRequest::to_json — limit null", "[network][messages_request]")
{
    NetworkMessagesRequest m;
    m.src = "a1";
    m.timestamp = 1000.0;

    JsonDocument doc = parse(m.to_json());
    REQUIRE(doc["type"] == "messages_request");
    REQUIRE(doc["src"] == "a1");
    REQUIRE(doc["limit"].isNull());
}

TEST_CASE("NetworkMessagesRequest::to_json — limit set", "[network][messages_request]")
{
    NetworkMessagesRequest m;
    m.src = "a1";
    m.timestamp = 1000.0;
    m.limit = 10;

    JsonDocument doc = parse(m.to_json());
    REQUIRE(doc["limit"] == 10);
}

TEST_CASE("NetworkMessagesRequest::from_json — canonical roundtrip", "[network][messages_request]")
{
    NetworkMessagesRequest m = NetworkMessagesRequest::from_json(MSGS_REQUEST_JSON);
    REQUIRE(m.src == "a1");
    REQUIRE(!m.limit.has_value());
    JsonDocument doc = parse(m.to_json());
    REQUIRE(doc["type"] == "messages_request");
    REQUIRE(doc["limit"].isNull());
}

TEST_CASE("NetworkMessagesRequest::from_json — missing limit field", "[network][messages_request]")
{
    // Older peers may omit "limit" — must still parse cleanly.
    const std::string json = R"({"type":"messages_request","dest":null,"src":"a1","isrc":null,)"
                             R"("timestamp":1000.0,"objuuid":null,"coluuid":null})";
    NetworkMessagesRequest m = NetworkMessagesRequest::from_json(json);
    REQUIRE(!m.limit.has_value());
}

TEST_CASE("network_message_type — messages_request", "[network][type_detection]")
{
    REQUIRE(network_message_type(MSGS_REQUEST_JSON) == NetworkMessageType::MessagesRequest);
}

// ═════════════════════════════════════════════════════════════════════════════
// Acknowledgement
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("Acknowledgement::to_json — ping ack", "[network][acknowledgement]")
{
    Acknowledgement a;
    a.ack_type = "ping";
    a.src = "a1";
    a.timestamp = 1000.0;

    JsonDocument doc = parse(a.to_json());
    REQUIRE(doc["type"] == "acknowledgement");
    REQUIRE(doc["ack_type"] == "ping");
    REQUIRE(doc["src"] == "a1");
    REQUIRE(doc["forwarded"].isNull());
    REQUIRE(doc["error"].isNull());
}

TEST_CASE("Acknowledgement::to_json — with error", "[network][acknowledgement]")
{
    Acknowledgement a;
    a.ack_type = "ticket_request";
    a.src = "a1";
    a.timestamp = 1000.0;
    a.error = "timeout";

    JsonDocument doc = parse(a.to_json());
    REQUIRE(doc["ack_type"] == "ticket_request");
    REQUIRE(doc["error"] == "timeout");
    REQUIRE(doc["forwarded"].isNull());
}

TEST_CASE("Acknowledgement::to_json — forwarded", "[network][acknowledgement]")
{
    Acknowledgement a;
    a.ack_type = "ping";
    a.src = "a1";
    a.timestamp = 1000.0;
    a.forwarded = "a2";

    JsonDocument doc = parse(a.to_json());
    REQUIRE(doc["forwarded"] == "a2");
    REQUIRE(doc["error"].isNull());
}

TEST_CASE("Acknowledgement::from_json — ping ack roundtrip", "[network][acknowledgement]")
{
    Acknowledgement a = Acknowledgement::from_json(ACK_PING_JSON);
    REQUIRE(a.ack_type == "ping");
    REQUIRE(a.src == "a1");
    REQUIRE(!a.forwarded.has_value());
    REQUIRE(!a.error.has_value());
    JsonDocument doc = parse(a.to_json());
    REQUIRE(doc["type"] == "acknowledgement");
}

TEST_CASE("Acknowledgement::from_json — error ack roundtrip", "[network][acknowledgement]")
{
    Acknowledgement a = Acknowledgement::from_json(ACK_ERROR_JSON);
    REQUIRE(a.ack_type == "ticket_request");
    REQUIRE(a.error.value() == "timeout");
}

TEST_CASE("Acknowledgement::from_json — forwarded roundtrip", "[network][acknowledgement]")
{
    Acknowledgement a = Acknowledgement::from_json(ACK_FORWARDED_JSON);
    REQUIRE(a.forwarded.value() == "a2");
    REQUIRE(!a.error.has_value());
}

TEST_CASE("network_message_type — acknowledgement", "[network][type_detection]")
{
    REQUIRE(network_message_type(ACK_PING_JSON) == NetworkMessageType::Acknowledgement);
}

// ═════════════════════════════════════════════════════════════════════════════
// NetworkMessagesResponse
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("NetworkMessagesResponse::to_json — empty messages", "[network][messages_response]")
{
    NetworkMessagesResponse r;
    r.src = "a1";
    r.timestamp = 1000.0;

    JsonDocument doc = parse(r.to_json());
    REQUIRE(doc["type"] == "messages_response");
    REQUIRE(doc["messages"].as<JsonArray>().size() == 0);
}

TEST_CASE("NetworkMessagesResponse::to_json — with embedded ping", "[network][messages_response]")
{
    Ping p;
    p.src = "b1";
    p.timestamp = 2000.0;

    NetworkMessagesResponse r;
    r.src = "a1";
    r.timestamp = 1000.0;
    r.message_jsons.push_back(p.to_json());

    JsonDocument doc = parse(r.to_json());
    REQUIRE(doc["messages"].as<JsonArray>().size() == 1);
    REQUIRE(doc["messages"][0]["type"] == "ping");
    REQUIRE(doc["messages"][0]["src"] == "b1");
    REQUIRE(doc["messages"][0]["timestamp"].as<double>() == Catch::Approx(2000.0));
}

TEST_CASE("NetworkMessagesResponse::from_json — empty roundtrip", "[network][messages_response]")
{
    NetworkMessagesResponse r = NetworkMessagesResponse::from_json(MSGS_RESP_EMPTY_JSON);
    REQUIRE(r.src == "a1");
    REQUIRE(r.message_jsons.empty());
    JsonDocument doc = parse(r.to_json());
    REQUIRE(doc["type"] == "messages_response");
    REQUIRE(doc["messages"].as<JsonArray>().size() == 0);
}

TEST_CASE("NetworkMessagesResponse::from_json — with ping roundtrip",
          "[network][messages_response]")
{
    NetworkMessagesResponse r = NetworkMessagesResponse::from_json(MSGS_RESP_WITH_PING_JSON);
    REQUIRE(r.message_jsons.size() == 1);
    JsonDocument inner = parse(r.message_jsons[0]);
    REQUIRE(inner["type"] == "ping");
    REQUIRE(inner["src"] == "b1");
    // Re-serialise the response and verify nested structure survives
    JsonDocument outer = parse(r.to_json());
    REQUIRE(outer["messages"][0]["type"] == "ping");
    REQUIRE(outer["messages"][0]["src"] == "b1");
}

TEST_CASE("network_message_type — messages_response", "[network][type_detection]")
{
    REQUIRE(network_message_type(MSGS_RESP_EMPTY_JSON) == NetworkMessageType::MessagesResponse);
}

// ═════════════════════════════════════════════════════════════════════════════
// TicketTraceResponse
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("TicketTraceResponse::to_json — basic", "[network][ticket_trace_response]")
{
    TicketTraceResponse t;
    t.tckuuid = "t1";
    t.network_ticket_type = "ticket_request";
    t.hop_time = 1000.0;
    t.src = "a1";
    t.timestamp = 1000.0;

    JsonDocument doc = parse(t.to_json());
    REQUIRE(doc["type"] == "ticket_trace_response");
    REQUIRE(doc["tckuuid"] == "t1");
    REQUIRE(doc["network_ticket_type"] == "ticket_request");
    REQUIRE(doc["hop_time"].as<double>() == Catch::Approx(1000.0));
    REQUIRE(doc["src"] == "a1");
}

TEST_CASE("TicketTraceResponse::from_json — canonical roundtrip",
          "[network][ticket_trace_response]")
{
    TicketTraceResponse t = TicketTraceResponse::from_json(TTR_JSON);
    REQUIRE(t.tckuuid == "t1");
    REQUIRE(t.network_ticket_type == "ticket_request");
    REQUIRE(t.hop_time == Catch::Approx(1000.0));
    REQUIRE(t.src == "a1");
    JsonDocument doc = parse(t.to_json());
    REQUIRE(doc["type"] == "ticket_trace_response");
}

TEST_CASE("TicketTraceResponse::hop — fields match", "[network][ticket_trace_response]")
{
    TicketTraceResponse t;
    t.src = "a1";
    t.hop_time = 42.5;
    t.network_ticket_type = "ticket_request";

    Hop h = t.hop();
    REQUIRE(h.agtuuid == "a1");
    REQUIRE(h.hop_time == Catch::Approx(42.5));
    REQUIRE(h.type_str == "ticket_request");
}

TEST_CASE("network_message_type — ticket_trace_response", "[network][type_detection]")
{
    REQUIRE(network_message_type(TTR_JSON) == NetworkMessageType::TicketTraceResponse);
}

// ═════════════════════════════════════════════════════════════════════════════
// NetworkTicket
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("NetworkTicket::from_json — ticket_request canonical roundtrip",
          "[network][network_ticket]")
{
    NetworkTicket t = NetworkTicket::from_json(NT_REQUEST_JSON);
    REQUIRE(t.tckuuid == "t1");
    REQUIRE(t.src == "a1");
    REQUIRE(!t.tracing);
    REQUIRE(!t.form_json.empty());
    JsonDocument form = parse(t.form_json);
    REQUIRE(form["type"] == "sync_process");
    REQUIRE(form["command"] == "ls /");
    REQUIRE(form["timeout"] == 15);
    // Re-serialise and verify type tag survives
    JsonDocument doc = parse(t.to_json("ticket_request"));
    REQUIRE(doc["type"] == "ticket_request");
    REQUIRE(doc["form"]["type"] == "sync_process");
}

TEST_CASE("NetworkTicket::from_json — ticket_response canonical roundtrip",
          "[network][network_ticket]")
{
    NetworkTicket t = NetworkTicket::from_json(NT_RESPONSE_JSON);
    REQUIRE(t.service_time.value() == Catch::Approx(0.5));
    JsonDocument form = parse(t.form_json);
    REQUIRE(form["stdout"] == "bin\n");
    REQUIRE(form["status"] == 0);
    REQUIRE(form["start_time"].as<double>() == Catch::Approx(1000.0));
}

TEST_CASE("network_message_type — ticket_request", "[network][type_detection]")
{
    REQUIRE(network_message_type(NT_REQUEST_JSON) == NetworkMessageType::TicketRequest);
}

TEST_CASE("network_message_type — ticket_response", "[network][type_detection]")
{
    REQUIRE(network_message_type(NT_RESPONSE_JSON) == NetworkMessageType::TicketResponse);
}

TEST_CASE("network_message_type — unknown", "[network][type_detection]")
{
    REQUIRE(network_message_type(R"({"type":"unknown_xyz"})") == NetworkMessageType::Unknown);
    REQUIRE(network_message_type(R"({})") == NetworkMessageType::Unknown);
}
