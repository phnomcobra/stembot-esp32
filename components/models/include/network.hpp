#pragma once

// Network message models — port of stembot-rust/src/models/network.rs.
//
// All optional fields serialise as JSON null to match the Python/Rust wire
// format.  Each struct's to_json() embeds the "type" tag in the output.
//
// Embedded sub-objects (the form inside NetworkTicket, the messages inside
// NetworkMessagesResponse) are stored as raw JSON strings so that the
// containing struct remains independent of the full type hierarchy, keeps
// ArduinoJson heap usage bounded, and lets the executor deserialise only the
// fields it actually needs.

#include "control.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// ── Hop ───────────────────────────────────────────────────────────────────────
// A single hop recorded when tracing a ticket through the network.
// Maps to Python's Hop(BaseModel) / Rust's models::control::Hop.

struct Hop
{
    std::string agtuuid;
    double hop_time = 0.0;
    std::string type_str;
};

// ── Ping ──────────────────────────────────────────────────────────────────────
// Wire type tag: "ping"

struct Ping
{
    std::string src;
    std::optional<std::string> dest;
    std::optional<std::string> isrc;
    std::optional<double> timestamp;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string to_json() const;
    static Ping from_json(const std::string& json);
};

// ── NetworkMessagesRequest ────────────────────────────────────────────────────
// Wire type tag: "messages_request"

struct NetworkMessagesRequest
{
    std::string src;
    std::optional<std::string> dest;
    std::optional<std::string> isrc;
    std::optional<double> timestamp;
    std::optional<int64_t> limit; // null means "no limit"
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string to_json() const;
    static NetworkMessagesRequest from_json(const std::string& json);
};

// ── Acknowledgement ───────────────────────────────────────────────────────────
// Wire type tag: "acknowledgement"

struct Acknowledgement
{
    std::string ack_type = "ping"; // e.g. "ping", "ticket_request"
    std::string src;
    std::optional<std::string> dest;
    std::optional<std::string> isrc;
    std::optional<double> timestamp;
    std::optional<std::string> forwarded;
    std::optional<std::string> error;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string to_json() const;
    static Acknowledgement from_json(const std::string& json);
};

// ── NetworkMessagesResponse ───────────────────────────────────────────────────
// Wire type tag: "messages_response"
//
// The embedded messages are stored as raw JSON strings (each is a full
// NetworkMessage JSON including its "type" tag) so that the outer struct does
// not depend on the full polymorphic type hierarchy.

struct NetworkMessagesResponse
{
    std::vector<std::string> message_jsons;
    std::string src;
    std::optional<std::string> dest;
    std::optional<std::string> isrc;
    std::optional<double> timestamp;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    std::string to_json() const;
    static NetworkMessagesResponse from_json(const std::string& json);
};

// ── TicketTraceResponse ───────────────────────────────────────────────────────
// Wire type tag: "ticket_trace_response"

struct TicketTraceResponse
{
    std::string tckuuid;
    std::string network_ticket_type;
    double hop_time = 0.0;
    std::string src;
    std::optional<std::string> dest;
    std::optional<std::string> isrc;
    std::optional<double> timestamp;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    // Returns a Hop representing this trace response.
    Hop hop() const;

    std::string to_json() const;
    static TicketTraceResponse from_json(const std::string& json);
};

// ── NetworkTicket ─────────────────────────────────────────────────────────────
// Used for both "ticket_request" and "ticket_response" — the caller supplies
// the type tag when serialising via to_json(type_tag).
//
// The embedded ControlForm is stored as a raw JSON string (form_json) that
// includes the form's "type" tag.

struct NetworkTicket
{
    std::string tckuuid;
    std::string form_json; // full JSON of the embedded ControlForm (with "type")
    bool tracing = false;
    std::string src;
    std::optional<std::string> dest;
    std::optional<std::string> isrc;
    std::optional<double> timestamp;
    std::optional<double> create_time;
    std::optional<double> service_time;
    std::optional<std::string> error;
    std::optional<std::string> objuuid;
    std::optional<std::string> coluuid;

    // type_tag: "ticket_request" or "ticket_response"
    std::string to_json(const std::string& type_tag) const;
    static NetworkTicket from_json(const std::string& json);
};

// ── NetworkMessageType ────────────────────────────────────────────────────────
// Discriminant for the NetworkMessage tagged union.

enum class NetworkMessageType
{
    Ping,
    MessagesRequest,
    MessagesResponse,
    Acknowledgement,
    TicketTraceResponse,
    TicketRequest,
    TicketResponse,
    Unknown
};

// Read the "type" field of a raw JSON string and return the corresponding
// NetworkMessageType.  Returns Unknown if the type tag is absent or
// unrecognised.  Does not fully deserialise the message.
NetworkMessageType network_message_type(const std::string& json);
