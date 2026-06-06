#include "control.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

// ═══════════════════════════════════════════════════════════════════════════════
// Minimal JSON serialisation helpers (internal linkage)
// ═══════════════════════════════════════════════════════════════════════════════

static std::string jesc(const std::string &s)
{
    std::string out;
    out.reserve(s.size() + 4);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

static std::string jstr(const std::string &s)
{
    return std::string(1, '"') + jesc(s) + '"';
}

static std::string jkv_str(const char *key, const std::optional<std::string> &v)
{
    std::string out = "\"";
    out += key;
    out += "\":";
    out += v ? jstr(*v) : "null";
    return out;
}

static std::string jkv_raw(const char *key, const std::optional<std::string> &raw)
{
    // Embeds a pre-built JSON fragment (e.g. a nested object) as a value.
    std::string out = "\"";
    out += key;
    out += "\":";
    out += raw ? *raw : "null";
    return out;
}

static std::string jkv_bool(const char *key, bool v)
{
    std::string out = "\"";
    out += key;
    out += "\":";
    out += v ? "true" : "false";
    return out;
}

static std::string jkv_i64(const char *key, const std::optional<int64_t> &v)
{
    std::string out = "\"";
    out += key;
    out += "\":";
    if (v) {
        out += std::to_string(*v);
    } else {
        out += "null";
    }
    return out;
}

static std::string jkv_f64(const char *key, const std::optional<double> &v)
{
    std::string out = "\"";
    out += key;
    out += "\":";
    if (v) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.6f", *v);
        out += buf;
    } else {
        out += "null";
    }
    return out;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Minimal JSON deserialisation helpers (internal linkage)
//
// These perform a linear scan for "key": <value>.  They are correct for
// well-formed, trusted protocol JSON with no duplicate keys.  A false match
// is only possible if a field name appears verbatim inside a string value —
// unlikely for the field names used by the stembot protocol.
// ═══════════════════════════════════════════════════════════════════════════════

// Advance pos past whitespace.
static void skip_ws(const std::string &s, size_t &pos)
{
    while (pos < s.size() && std::isspace((unsigned char)s[pos])) {
        ++pos;
    }
}

// Locate "key": in s starting at pos and advance pos to the first character
// of the value.  Returns false if the key is not found.
static bool find_key(const std::string &s, const char *key, size_t &pos)
{
    const std::string needle = std::string("\"") + key + "\"";
    size_t            found  = s.find(needle, pos);
    if (found == std::string::npos) {
        return false;
    }
    pos = found + needle.size();
    skip_ws(s, pos);
    if (pos >= s.size() || s[pos] != ':') {
        return false;
    }
    ++pos; // skip ':'
    skip_ws(s, pos);
    return true;
}

static std::optional<std::string> jget_str(const std::string &s, const char *key)
{
    size_t pos = 0;
    if (!find_key(s, key, pos)) {
        return std::nullopt;
    }
    if (s[pos] == 'n') {
        return std::nullopt; // null
    }
    if (s[pos] != '"') {
        return std::nullopt;
    }
    ++pos; // skip opening '"'
    std::string result;
    while (pos < s.size() && s[pos] != '"') {
        if (s[pos] == '\\' && pos + 1 < s.size()) {
            ++pos;
            switch (s[pos]) {
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                default:   result += s[pos]; break;
            }
        } else {
            result += s[pos];
        }
        ++pos;
    }
    return result;
}

static std::optional<bool> jget_bool(const std::string &s, const char *key)
{
    size_t pos = 0;
    if (!find_key(s, key, pos)) {
        return std::nullopt;
    }
    if (pos >= s.size() || s[pos] == 'n') {
        return std::nullopt;
    }
    if (s[pos] == 't') {
        return true;
    }
    if (s[pos] == 'f') {
        return false;
    }
    return std::nullopt;
}

static std::optional<int64_t> jget_i64(const std::string &s, const char *key)
{
    size_t pos = 0;
    if (!find_key(s, key, pos)) {
        return std::nullopt;
    }
    if (pos >= s.size() || s[pos] == 'n') {
        return std::nullopt;
    }
    if (s[pos] == '-' || std::isdigit((unsigned char)s[pos])) {
        size_t end = pos;
        if (s[end] == '-') {
            ++end;
        }
        while (end < s.size() && std::isdigit((unsigned char)s[end])) {
            ++end;
        }
        try {
            return static_cast<int64_t>(std::stoll(s.substr(pos, end - pos)));
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

static std::optional<double> jget_f64(const std::string &s, const char *key)
{
    size_t pos = 0;
    if (!find_key(s, key, pos)) {
        return std::nullopt;
    }
    if (pos >= s.size() || s[pos] == 'n') {
        return std::nullopt;
    }
    if (s[pos] == '-' || std::isdigit((unsigned char)s[pos])) {
        size_t end = pos;
        if (s[end] == '-') {
            ++end;
        }
        while (end < s.size() &&
               (std::isdigit((unsigned char)s[end]) || s[end] == '.' ||
                s[end] == 'e' || s[end] == 'E' || s[end] == '+' ||
                s[end] == '-')) {
            ++end;
        }
        try {
            return std::stod(s.substr(pos, end - pos));
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

// Extract the raw JSON text of a nested object value (handles nesting and
// string contents so braces inside quoted strings are ignored).
static std::optional<std::string> jget_object(const std::string &s, const char *key)
{
    size_t pos = 0;
    if (!find_key(s, key, pos)) {
        return std::nullopt;
    }
    if (pos >= s.size() || s[pos] == 'n') {
        return std::nullopt;
    }
    if (s[pos] != '{') {
        return std::nullopt;
    }
    size_t start  = pos;
    int    depth  = 0;
    bool   in_str = false;
    while (pos < s.size()) {
        char c = s[pos];
        if (in_str) {
            if (c == '\\') {
                ++pos; // skip escaped character
            } else if (c == '"') {
                in_str = false;
            }
        } else {
            if (c == '"') {
                in_str = true;
            } else if (c == '{') {
                ++depth;
            } else if (c == '}') {
                --depth;
                if (depth == 0) {
                    ++pos;
                    return s.substr(start, pos - start);
                }
            }
        }
        ++pos;
    }
    return std::nullopt;
}

// Extract each JSON object from an array value for a given key.
static std::vector<std::string> jget_array_objects(const std::string &s, const char *key)
{
    std::vector<std::string> result;
    size_t                   pos = 0;
    if (!find_key(s, key, pos)) {
        return result;
    }
    if (pos >= s.size() || s[pos] != '[') {
        return result;
    }
    ++pos; // skip '['
    while (pos < s.size()) {
        skip_ws(s, pos);
        if (pos >= s.size() || s[pos] == ']') {
            break;
        }
        if (s[pos] == ',') {
            ++pos;
            continue;
        }
        if (s[pos] == '{') {
            size_t start  = pos;
            int    depth  = 0;
            bool   in_str = false;
            while (pos < s.size()) {
                char c = s[pos];
                if (in_str) {
                    if (c == '\\') {
                        ++pos;
                    } else if (c == '"') {
                        in_str = false;
                    }
                } else {
                    if (c == '"') {
                        in_str = true;
                    } else if (c == '{') {
                        ++depth;
                    } else if (c == '}') {
                        --depth;
                        if (depth == 0) {
                            ++pos;
                            result.push_back(s.substr(start, pos - start));
                            break;
                        }
                    }
                }
                ++pos;
            }
        } else {
            ++pos; // skip unexpected character
        }
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Peer
// ═══════════════════════════════════════════════════════════════════════════════

std::string Peer::to_json() const
{
    std::string out = "{";
    out += jkv_str("agtuuid", agtuuid);      out += ',';
    out += jkv_bool("polling", polling);     out += ',';
    out += jkv_f64("destroy_time", destroy_time); out += ',';
    out += jkv_f64("refresh_time", refresh_time); out += ',';
    out += jkv_str("url", url);              out += ',';
    out += jkv_str("objuuid", objuuid);      out += ',';
    out += jkv_str("coluuid", coluuid);
    out += '}';
    return out;
}

Peer Peer::from_json(const std::string &json)
{
    Peer p;
    p.agtuuid      = jget_str(json,  "agtuuid");
    p.polling      = jget_bool(json, "polling").value_or(false);
    p.destroy_time = jget_f64(json,  "destroy_time");
    p.refresh_time = jget_f64(json,  "refresh_time");
    p.url          = jget_str(json,  "url");
    p.objuuid      = jget_str(json,  "objuuid");
    p.coluuid      = jget_str(json,  "coluuid");
    return p;
}

// ═══════════════════════════════════════════════════════════════════════════════
// GetConfig
// ═══════════════════════════════════════════════════════════════════════════════

std::string GetConfig::to_json() const
{
    std::string out = "{";
    out += "\"type\":\"get_config\",";
    out += jkv_raw("config", config_json); out += ',';
    out += jkv_str("error", error);        out += ',';
    out += jkv_str("objuuid", objuuid);    out += ',';
    out += jkv_str("coluuid", coluuid);
    out += '}';
    return out;
}

GetConfig GetConfig::from_json(const std::string &json)
{
    GetConfig f;
    f.config_json = jget_object(json, "config");
    f.error       = jget_str(json,   "error");
    f.objuuid     = jget_str(json,   "objuuid");
    f.coluuid     = jget_str(json,   "coluuid");
    return f;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Benchmark
// ═══════════════════════════════════════════════════════════════════════════════

std::string Benchmark::to_json() const
{
    std::string out = "{";
    out += "\"type\":\"benchmark\",";
    out += jkv_i64("outbound_size", outbound_size); out += ',';
    out += jkv_i64("inbound_size", inbound_size);   out += ',';
    out += jkv_str("payload", payload);              out += ',';
    out += jkv_str("error", error);                  out += ',';
    out += jkv_str("objuuid", objuuid);              out += ',';
    out += jkv_str("coluuid", coluuid);
    out += '}';
    return out;
}

Benchmark Benchmark::from_json(const std::string &json)
{
    Benchmark f;
    f.outbound_size = jget_i64(json, "outbound_size");
    f.inbound_size  = jget_i64(json, "inbound_size");
    f.payload       = jget_str(json, "payload");
    f.error         = jget_str(json, "error");
    f.objuuid       = jget_str(json, "objuuid");
    f.coluuid       = jget_str(json, "coluuid");
    return f;
}

// ═══════════════════════════════════════════════════════════════════════════════
// GetPeers
// ═══════════════════════════════════════════════════════════════════════════════

std::string GetPeers::to_json() const
{
    std::string out = "{";
    out += "\"type\":\"get_peers\",";
    out += "\"peers\":[";
    for (size_t i = 0; i < peers.size(); ++i) {
        if (i > 0) {
            out += ',';
        }
        out += peers[i].to_json();
    }
    out += "],";
    out += jkv_str("error", error);   out += ',';
    out += jkv_str("objuuid", objuuid); out += ',';
    out += jkv_str("coluuid", coluuid);
    out += '}';
    return out;
}

GetPeers GetPeers::from_json(const std::string &json)
{
    GetPeers f;
    for (const std::string &peer_json : jget_array_objects(json, "peers")) {
        f.peers.push_back(Peer::from_json(peer_json));
    }
    f.error   = jget_str(json, "error");
    f.objuuid = jget_str(json, "objuuid");
    f.coluuid = jget_str(json, "coluuid");
    return f;
}

