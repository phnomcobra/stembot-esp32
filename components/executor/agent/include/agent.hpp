#pragma once

// AgentClient — HTTP client for secure inter-agent communication.
//
// Wire format (mirrors Python stembot/executor/agent.py and Rust src/executor/agent.rs):
//   POST {url}
//   Content-Type:   application/binary
//   Nonce:          hex(16-byte random nonce)
//   Tag:            hex(16-byte AES-EAX MAC tag)
//   Body:           AES-256-EAX ciphertext
//
// All messages are AES-256-EAX encrypted using the 32-byte key stored in the
// Config KV-store.  send_network_message() sets "isrc" to the local agent UUID
// before encrypting.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// ── AES-256-EAX result ────────────────────────────────────────────────────────

struct AesEaxResult
{
    uint8_t nonce[16]; // random nonce used for this encryption
    uint8_t tag[16];   // AES-EAX MAC tag (16 bytes)
    std::vector<uint8_t> ciphertext;
};

// ── Crypto primitives ─────────────────────────────────────────────────────────

// Encrypt plaintext with AES-256-EAX.
// Returns AesEaxResult; on failure ciphertext will be empty.
AesEaxResult aes_eax_encrypt(const uint8_t key[32], const uint8_t* data, size_t len);

// Decrypt ciphertext with AES-256-EAX.
// Returns the plaintext, or nullopt if MAC verification fails.
std::optional<std::vector<uint8_t>> aes_eax_decrypt(
    const uint8_t key[32],
    const uint8_t nonce[16],
    const uint8_t tag[16],
    const uint8_t* ciphertext,
    size_t ct_len);

// ── Wire-format utilities ─────────────────────────────────────────────────────

// Encode bytes as a lowercase hex string.
std::string bytes_to_hex(const uint8_t* data, size_t len);

// Decode a hex string to bytes. Returns empty vector on invalid input.
std::vector<uint8_t> hex_to_bytes(const std::string& hex);

// Set the "isrc" field in a raw network-message JSON string.
// All NetworkMessage types carry an isrc field; this modifies it in-place.
std::string set_isrc_json(const std::string& msg_json, const std::string& isrc);

// ── AgentClient ───────────────────────────────────────────────────────────────

class AgentClient
{
public:
    // Construct with explicit credentials (mirrors Rust with_credentials).
    AgentClient(std::string url, const uint8_t key[32], std::string agtuuid);

    // Send an encrypted control-form JSON and return the decrypted response
    // JSON.  Returns an empty string on any error.
    std::string send_control_form(const std::string& form_json);

    // Send an encrypted network-message JSON and return the decrypted
    // response JSON.  Automatically sets isrc to agtuuid before encrypting.
    // Returns an empty string on any error.
    std::string send_network_message(const std::string& msg_json);

private:
    std::string url_;
    uint8_t     key_[32];
    std::string agtuuid_;

    // Encrypt plaintext_json, POST to url_, decrypt response.
    std::string post_encrypted(const std::string& plaintext_json);
};