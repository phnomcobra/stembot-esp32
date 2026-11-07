#pragma once

#include <cstdint>
#include <string>

// Derive the v1 user key shared by the Python, Rust, and ESP32 agents.
// Returns false when the cryptographic operation fails.
bool derive_user_key(uint8_t out[32], const std::string& passphrase, const std::string& objuuid);
