#include "kvstore.hpp"

#include "esp_log.h"

#include <cstring>

static const char* TAG = "KVStore";

// ── Lifecycle ──────────────────────────────────────────────

KVStore::KVStore() : _handle(0), _started(false), _readOnly(false)
{
}

KVStore::~KVStore()
{
    end();
}

bool KVStore::begin(const char* name, bool readOnly)
{
    if (_started)
    {
        return false;
    }
    _readOnly = readOnly;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_flash_init: %s", esp_err_to_name(err));
        return false;
    }

    err = nvs_open(name, readOnly ? NVS_READONLY : NVS_READWRITE, &_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_open(\"%s\"): %s", name, esp_err_to_name(err));
        return false;
    }

    _started = true;
    return true;
}

void KVStore::end()
{
    if (!_started)
    {
        return;
    }
    nvs_close(_handle);
    _handle = 0;
    _started = false;
}

// ── Namespace operations ───────────────────────────────────

bool KVStore::clear()
{
    if (!_started || _readOnly)
    {
        return false;
    }
    if (nvs_erase_all(_handle) != ESP_OK)
    {
        return false;
    }
    return nvs_commit(_handle) == ESP_OK;
}

bool KVStore::remove(const char* key)
{
    if (!_started || _readOnly)
    {
        return false;
    }
    if (nvs_erase_key(_handle, key) != ESP_OK)
    {
        return false;
    }
    return nvs_commit(_handle) == ESP_OK;
}

bool KVStore::isKey(const char* key)
{
    if (!_started)
    {
        return false;
    }
    nvs_type_t type = NVS_TYPE_ANY;
    return nvs_find_key(_handle, key, &type) == ESP_OK;
}

// ── put — scalar integers ──────────────────────────────────

size_t KVStore::putChar(const char* key, int8_t value)
{
    if (!_started || _readOnly)
    {
        return 0;
    }
    if (nvs_set_i8(_handle, key, value) != ESP_OK)
    {
        return 0;
    }
    nvs_commit(_handle);
    return sizeof(value);
}

size_t KVStore::putUChar(const char* key, uint8_t value)
{
    if (!_started || _readOnly)
    {
        return 0;
    }
    if (nvs_set_u8(_handle, key, value) != ESP_OK)
    {
        return 0;
    }
    nvs_commit(_handle);
    return sizeof(value);
}

size_t KVStore::putShort(const char* key, int16_t value)
{
    if (!_started || _readOnly)
    {
        return 0;
    }
    if (nvs_set_i16(_handle, key, value) != ESP_OK)
    {
        return 0;
    }
    nvs_commit(_handle);
    return sizeof(value);
}

size_t KVStore::putUShort(const char* key, uint16_t value)
{
    if (!_started || _readOnly)
    {
        return 0;
    }
    if (nvs_set_u16(_handle, key, value) != ESP_OK)
    {
        return 0;
    }
    nvs_commit(_handle);
    return sizeof(value);
}

size_t KVStore::putInt(const char* key, int32_t value)
{
    if (!_started || _readOnly)
    {
        return 0;
    }
    if (nvs_set_i32(_handle, key, value) != ESP_OK)
    {
        return 0;
    }
    nvs_commit(_handle);
    return sizeof(value);
}

size_t KVStore::putUInt(const char* key, uint32_t value)
{
    if (!_started || _readOnly)
    {
        return 0;
    }
    if (nvs_set_u32(_handle, key, value) != ESP_OK)
    {
        return 0;
    }
    nvs_commit(_handle);
    return sizeof(value);
}

size_t KVStore::putLong(const char* key, int32_t value)
{
    return putInt(key, value);
}

size_t KVStore::putULong(const char* key, uint32_t value)
{
    return putUInt(key, value);
}

size_t KVStore::putLong64(const char* key, int64_t value)
{
    if (!_started || _readOnly)
    {
        return 0;
    }
    if (nvs_set_i64(_handle, key, value) != ESP_OK)
    {
        return 0;
    }
    nvs_commit(_handle);
    return sizeof(value);
}

size_t KVStore::putULong64(const char* key, uint64_t value)
{
    if (!_started || _readOnly)
    {
        return 0;
    }
    if (nvs_set_u64(_handle, key, value) != ESP_OK)
    {
        return 0;
    }
    nvs_commit(_handle);
    return sizeof(value);
}

// ── put — floating point (stored as blobs) ─────────────────

size_t KVStore::putFloat(const char* key, float value)
{
    return putBytes(key, &value, sizeof(value));
}

size_t KVStore::putDouble(const char* key, double value)
{
    return putBytes(key, &value, sizeof(value));
}

// ── put — bool / string / blob ─────────────────────────────

size_t KVStore::putBool(const char* key, bool value)
{
    return putUChar(key, value ? 1U : 0U);
}

size_t KVStore::putString(const char* key, const char* value)
{
    if (!_started || _readOnly || (value == nullptr))
    {
        return 0;
    }
    if (nvs_set_str(_handle, key, value) != ESP_OK)
    {
        return 0;
    }
    nvs_commit(_handle);
    return strlen(value);
}

size_t KVStore::putString(const char* key, const std::string& value)
{
    return putString(key, value.c_str());
}

size_t KVStore::putBytes(const char* key, const void* value, size_t len)
{
    if (!_started || _readOnly || (value == nullptr) || (len == 0U))
    {
        return 0;
    }
    if (nvs_set_blob(_handle, key, value, len) != ESP_OK)
    {
        return 0;
    }
    nvs_commit(_handle);
    return len;
}

// ── get — scalar integers ──────────────────────────────────

int8_t KVStore::getChar(const char* key, int8_t defaultValue)
{
    if (!_started)
    {
        return defaultValue;
    }
    int8_t val = defaultValue;
    nvs_get_i8(_handle, key, &val);
    return val;
}

uint8_t KVStore::getUChar(const char* key, uint8_t defaultValue)
{
    if (!_started)
    {
        return defaultValue;
    }
    uint8_t val = defaultValue;
    nvs_get_u8(_handle, key, &val);
    return val;
}

int16_t KVStore::getShort(const char* key, int16_t defaultValue)
{
    if (!_started)
    {
        return defaultValue;
    }
    int16_t val = defaultValue;
    nvs_get_i16(_handle, key, &val);
    return val;
}

uint16_t KVStore::getUShort(const char* key, uint16_t defaultValue)
{
    if (!_started)
    {
        return defaultValue;
    }
    uint16_t val = defaultValue;
    nvs_get_u16(_handle, key, &val);
    return val;
}

int32_t KVStore::getInt(const char* key, int32_t defaultValue)
{
    if (!_started)
    {
        return defaultValue;
    }
    int32_t val = defaultValue;
    nvs_get_i32(_handle, key, &val);
    return val;
}

uint32_t KVStore::getUInt(const char* key, uint32_t defaultValue)
{
    if (!_started)
    {
        return defaultValue;
    }
    uint32_t val = defaultValue;
    nvs_get_u32(_handle, key, &val);
    return val;
}

int32_t KVStore::getLong(const char* key, int32_t defaultValue)
{
    return getInt(key, defaultValue);
}

uint32_t KVStore::getULong(const char* key, uint32_t defaultValue)
{
    return getUInt(key, defaultValue);
}

int64_t KVStore::getLong64(const char* key, int64_t defaultValue)
{
    if (!_started)
    {
        return defaultValue;
    }
    int64_t val = defaultValue;
    nvs_get_i64(_handle, key, &val);
    return val;
}

uint64_t KVStore::getULong64(const char* key, uint64_t defaultValue)
{
    if (!_started)
    {
        return defaultValue;
    }
    uint64_t val = defaultValue;
    nvs_get_u64(_handle, key, &val);
    return val;
}

// ── get — floating point ───────────────────────────────────

float KVStore::getFloat(const char* key, float defaultValue)
{
    float val = defaultValue;
    getBytes(key, &val, sizeof(val));
    return val;
}

double KVStore::getDouble(const char* key, double defaultValue)
{
    double val = defaultValue;
    getBytes(key, &val, sizeof(val));
    return val;
}

// ── get — bool / string / blob ─────────────────────────────

bool KVStore::getBool(const char* key, bool defaultValue)
{
    return getUChar(key, defaultValue ? 1U : 0U) != 0U;
}

std::string KVStore::getString(const char* key, const std::string& defaultValue)
{
    if (!_started)
    {
        return defaultValue;
    }
    size_t len = 0;
    if (nvs_get_str(_handle, key, nullptr, &len) != ESP_OK)
    {
        return defaultValue;
    }
    std::string result(len, '\0');
    if (nvs_get_str(_handle, key, result.data(), &len) != ESP_OK)
    {
        return defaultValue;
    }
    // nvs_get_str includes the null terminator in len; strip it.
    if (!result.empty() && (result.back() == '\0'))
    {
        result.pop_back();
    }
    return result;
}

size_t KVStore::getBytesLength(const char* key)
{
    if (!_started)
    {
        return 0;
    }
    size_t len = 0;
    nvs_get_blob(_handle, key, nullptr, &len);
    return len;
}

size_t KVStore::getBytes(const char* key, void* buf, size_t maxLen)
{
    if (!_started || (buf == nullptr) || (maxLen == 0U))
    {
        return 0;
    }
    size_t len = maxLen;
    if (nvs_get_blob(_handle, key, buf, &len) != ESP_OK)
    {
        return 0;
    }
    return len;
}
