#pragma once

#include "nvs.h"
#include "nvs_flash.h"

#include <cstddef>
#include <cstdint>
#include <string>

// KVStore — a thin C++ wrapper around the ESP-IDF NVS API.
//
// Mirrors the interface of the Arduino ESP32 Preferences library so that code
// written for the Arduino framework can be ported with minimal changes.
//
// Usage:
//   KVStore kvstore;
//   kvstore.begin("my-app");
//   kvstore.putInt("count", 42);
//   int n = kvstore.getInt("count", 0);
//   kvstore.end();
class KVStore
{
  public:
    KVStore();
    ~KVStore();

    // Open an NVS namespace.  Call end() when finished.
    // Returns false if the namespace cannot be opened.
    bool begin(const char* name, bool readOnly = false);
    void end();

    // Erase all keys in the open namespace.
    bool clear();

    // Remove a single key from the open namespace.
    bool remove(const char* key);

    // Returns true if the key exists in the open namespace.
    bool isKey(const char* key);

    // ── Scalar put ─────────────────────────────────────────
    size_t putChar(const char* key, int8_t value);
    size_t putUChar(const char* key, uint8_t value);
    size_t putShort(const char* key, int16_t value);
    size_t putUShort(const char* key, uint16_t value);
    size_t putInt(const char* key, int32_t value);
    size_t putUInt(const char* key, uint32_t value);
    size_t putLong(const char* key, int32_t value);
    size_t putULong(const char* key, uint32_t value);
    size_t putLong64(const char* key, int64_t value);
    size_t putULong64(const char* key, uint64_t value);
    size_t putFloat(const char* key, float value);
    size_t putDouble(const char* key, double value);
    size_t putBool(const char* key, bool value);
    size_t putString(const char* key, const char* value);
    size_t putString(const char* key, const std::string& value);
    size_t putBytes(const char* key, const void* value, size_t len);

    // ── Scalar get ─────────────────────────────────────────
    int8_t getChar(const char* key, int8_t defaultValue = 0);
    uint8_t getUChar(const char* key, uint8_t defaultValue = 0);
    int16_t getShort(const char* key, int16_t defaultValue = 0);
    uint16_t getUShort(const char* key, uint16_t defaultValue = 0);
    int32_t getInt(const char* key, int32_t defaultValue = 0);
    uint32_t getUInt(const char* key, uint32_t defaultValue = 0);
    int32_t getLong(const char* key, int32_t defaultValue = 0);
    uint32_t getULong(const char* key, uint32_t defaultValue = 0);
    int64_t getLong64(const char* key, int64_t defaultValue = 0);
    uint64_t getULong64(const char* key, uint64_t defaultValue = 0);
    float getFloat(const char* key, float defaultValue = 0.0F);
    double getDouble(const char* key, double defaultValue = 0.0);
    bool getBool(const char* key, bool defaultValue = false);
    std::string getString(const char* key, const std::string& defaultValue = "");

    // Returns the length in bytes of a blob value, or 0 if not found.
    size_t getBytesLength(const char* key);

    // Copies up to maxLen bytes into buf.  Returns the number of bytes read.
    size_t getBytes(const char* key, void* buf, size_t maxLen);

  private:
    nvs_handle_t _handle;
    bool _started;
    bool _readOnly;
};
