#include "config.hpp"

#include "psa/crypto.h"

#include <cstring>

// Matches the stembot-python default:
//   hashlib.sha256(b'changeme').digest()[:32]
static void default_key(uint8_t out[32])
{
    static const char passphrase[] = "changeme";
    size_t hash_len = 0;
    psa_crypto_init();
    psa_hash_compute(PSA_ALG_SHA_256, reinterpret_cast<const uint8_t*>(passphrase),
                     sizeof(passphrase) - 1, // exclude null terminator
                     out, 32, &hash_len);
}

static void set_key_with_passphrase(uint8_t out[32], const std::string& passphrase)
{
    size_t hash_len = 0;
    psa_crypto_init();
    psa_hash_compute(PSA_ALG_SHA_256, reinterpret_cast<const uint8_t*>(passphrase.data()),
                     passphrase.size(), out, 32, &hash_len);
}

void Config::set_passphrase(const std::string& passphrase)
{
    set_key_with_passphrase(key, passphrase);
}

Config::Config()
{
    Config::_kvstore.begin("config");
    strncpy(agtuuid,
            Config::_kvstore.getString("agtuuid", "00000000-0000-0000-0000-000000000000").c_str(),
            sizeof(agtuuid));

    std::string stored_key = Config::_kvstore.getString("key", "");
    if (stored_key.empty())
    {
        default_key(key);
    }
    else
    {
        memcpy(key, stored_key.data(), std::min(stored_key.size(), sizeof(key)));
    }

    Config::peerUrl = Config::_kvstore.getString("peer_url", "");
    Config::wifiSSID = Config::_kvstore.getString("wifi_ssid", "");
    Config::wifiPassword = Config::_kvstore.getString("wifi_password", "");
}

Config::~Config()
{
    Config::_kvstore.end();
}

void Config::save()
{
    Config::_kvstore.putString("agtuuid", agtuuid);
    Config::_kvstore.putBytes("key", key, sizeof(key));
    Config::_kvstore.putString("peer_url", peerUrl);
    Config::_kvstore.putString("wifi_ssid", wifiSSID);
    Config::_kvstore.putString("wifi_password", wifiPassword);
}
