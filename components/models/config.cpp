#include "config.hpp"

#include "psa/crypto.h"
#include <ArduinoJson.h>

#include <cstring>

// Matches the stembot-python default:
//   hashlib.sha256(b'changeme').digest()[:32]
static void default_key(uint8_t out[32])
{
    static const char passphrase[] = "changeme";
    size_t hash_len = 0;
    psa_crypto_init();
    const psa_status_t st = psa_hash_compute(
        PSA_ALG_SHA_256, reinterpret_cast<const uint8_t*>(passphrase), sizeof(passphrase) - 1,
        out, 32, &hash_len);
    if (st != PSA_SUCCESS || hash_len != 32)
        memset(out, 0, 32);
}

static void set_key_with_passphrase(uint8_t out[32], const std::string& passphrase)
{
    size_t hash_len = 0;
    psa_crypto_init();
    const psa_status_t st = psa_hash_compute(
        PSA_ALG_SHA_256, reinterpret_cast<const uint8_t*>(passphrase.data()), passphrase.size(),
        out, 32, &hash_len);
    if (st != PSA_SUCCESS || hash_len != 32)
        memset(out, 0, 32);
}

void Config::set_passphrase(const std::string& passphrase)
{
    set_key_with_passphrase(key, passphrase);
}

Config::Config()
{
    load();
}


Config::~Config()
{
    Config::_kvstore.end();
}

void Config::load()
{
    Config::_kvstore.begin("config");
    strncpy(agtuuid,
            Config::_kvstore.getString("agtuuid", "00000000-0000-0000-0000-000000000000").c_str(),
            sizeof(agtuuid));

    size_t key_len = Config::_kvstore.getBytes("key", key, sizeof(key));
    if (key_len != sizeof(key))
    {
        default_key(key);
    }

    Config::peerUrl = Config::_kvstore.getString("peer_url", "");
    Config::wifiSSID = Config::_kvstore.getString("wifi_ssid", "");
    Config::wifiPassword = Config::_kvstore.getString("wifi_password", "");
    Config::debug = Config::_kvstore.getBool("debug", false);
    Config::polling = Config::_kvstore.getBool("polling", false);
}

void Config::save()
{
    Config::_kvstore.putString("agtuuid", agtuuid);
    Config::_kvstore.putBytes("key", key, sizeof(key));
    Config::_kvstore.putString("peer_url", peerUrl);
    Config::_kvstore.putString("wifi_ssid", wifiSSID);
    Config::_kvstore.putString("wifi_password", wifiPassword);
    Config::_kvstore.putBool("debug", debug);
    Config::_kvstore.putBool("polling", polling);
}

std::string Config::to_json() const
{
    JsonDocument doc;

    doc["agtuuid"] = Config::agtuuid;
    doc["peerUrl"] = Config::peerUrl;
    doc["wifiSSID"] = Config::wifiSSID;
    doc["debug"] = Config::debug;
    doc["polling"] = Config::polling;

    std::string json;
    serializeJson(doc, json);
    return json;
}