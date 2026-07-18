// AgentClient implementation using esp_http_client.
// Encrypts outgoing messages with AES-256-EAX (aes_eax_encrypt) and
// decrypts incoming responses (aes_eax_decrypt).  The Nonce and Tag are
// transmitted as lowercase hex strings in HTTP headers.

#include "agent.hpp"

#include "esp_http_client.h"
#include "esp_log.h"

#include <cstring>
#include <strings.h>
#include <vector>

static const char* TAG = "agent";

// ── HTTP event handler ────────────────────────────────────────────────────────

struct HttpResponse
{
    std::string nonce_hex;
    std::string tag_hex;
    std::vector<uint8_t> body;
    bool error = false;
};

static esp_err_t http_event_handler(esp_http_client_event_t* evt)
{
    auto* resp = static_cast<HttpResponse*>(evt->user_data);
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_HEADER:
        if (strcasecmp(evt->header_key, "Nonce") == 0)
            resp->nonce_hex = evt->header_value;
        else if (strcasecmp(evt->header_key, "Tag") == 0)
            resp->tag_hex = evt->header_value;
        break;
    case HTTP_EVENT_ON_DATA:
    {
        const auto* begin = static_cast<const uint8_t*>(evt->data);
        resp->body.insert(resp->body.end(), begin, begin + evt->data_len);
        break;
    }
    case HTTP_EVENT_ERROR:
        resp->error = true;
        break;
    default:
        break;
    }
    return ESP_OK;
}

// ── AgentClient ───────────────────────────────────────────────────────────────

std::string AgentClient::send_control_form(const std::string& form_json)
{
    return post_encrypted(form_json);
}

std::string AgentClient::send_network_message(const std::string& msg_json)
{
    return post_encrypted(set_isrc_json(msg_json, agtuuid_));
}

std::string AgentClient::post_encrypted(const std::string& plaintext_json)
{
    AesEaxResult enc = aes_eax_encrypt(
        key_, reinterpret_cast<const uint8_t*>(plaintext_json.data()), plaintext_json.size());

    if (enc.ciphertext.empty() && !plaintext_json.empty())
    {
        ESP_LOGE(TAG, "Encryption failed");
        return "";
    }

    const std::string nonce_hex = bytes_to_hex(enc.nonce, 16);
    const std::string tag_hex = bytes_to_hex(enc.tag, 16);

    HttpResponse resp;

    esp_http_client_config_t cfg = {};
    cfg.url = url_.c_str();
    cfg.event_handler = http_event_handler;
    cfg.user_data = &resp;
    cfg.timeout_ms = 30000;

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client)
        return "";

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/binary");
    esp_http_client_set_header(client, "Nonce", nonce_hex.c_str());
    esp_http_client_set_header(client, "Tag", tag_hex.c_str());
    esp_http_client_set_post_field(client, reinterpret_cast<const char*>(enc.ciphertext.data()),
                                   static_cast<int>(enc.ciphertext.size()));

    const esp_err_t err = esp_http_client_perform(client);
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status < 200 || status >= 300 || resp.error)
    {
        ESP_LOGE(TAG, "HTTP error: %s, status=%d", esp_err_to_name(err), status);
        return "";
    }

    if (resp.nonce_hex.size() != 32 || resp.tag_hex.size() != 32)
    {
        ESP_LOGE(TAG, "Response missing or malformed Nonce/Tag header");
        return "";
    }

    const auto resp_nonce = hex_to_bytes(resp.nonce_hex);
    const auto resp_tag = hex_to_bytes(resp.tag_hex);

    if (resp_nonce.size() != 16 || resp_tag.size() != 16)
    {
        ESP_LOGE(TAG, "Response Nonce/Tag wrong length");
        return "";
    }

    auto plain = aes_eax_decrypt(key_, resp_nonce.data(), resp_tag.data(), resp.body.data(),
                                 resp.body.size());
    if (!plain)
    {
        ESP_LOGE(TAG, "Response authentication failed");
        return "";
    }

    return std::string(plain->begin(), plain->end());
}
