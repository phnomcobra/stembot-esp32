#include "config.h"

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
}

Config::~Config()
{
    Config::_kvstore.end();
}
