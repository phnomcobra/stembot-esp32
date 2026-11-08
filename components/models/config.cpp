#include "config.hpp"

#include "crypto.hpp"

#include <ArduinoJson.h>
#include <cstring>

void Config::set_user_key(const std::string& uuid, const std::string& passphrase)
{
    derive_user_key(key, passphrase, uuid);
    strncpy(keyuuid, uuid.c_str(), sizeof(keyuuid) - 1);
    keyuuid[sizeof(keyuuid) - 1] = '\0';
    keytype = KeyType::USER;
    keyExpireTime = user_key_expire_time();
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
    agtuuid[sizeof(agtuuid) - 1] = '\0';
    strncpy(keyuuid,
            Config::_kvstore.getString("keyuuid", "00000000-0000-0000-0000-000000000000").c_str(),
            sizeof(keyuuid));
    keyuuid[sizeof(keyuuid) - 1] = '\0';

    size_t key_len = Config::_kvstore.getBytes("key", key, sizeof(key));
    if (key_len != sizeof(key))
    {
        memset(key, 0, sizeof(key));
    }
    keytype = key_type_from_string(Config::_kvstore.getString("key_type", "user"));
    keyExpireTime = Config::_kvstore.getDouble("key_expire_time", 0.0);

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
    Config::_kvstore.putString("keyuuid", keyuuid);
    Config::_kvstore.putString("key_type", key_type_to_string(keytype));
    Config::_kvstore.putDouble("key_expire_time", keyExpireTime);
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
    doc["keyuuid"] = Config::keyuuid;
    doc["peerUrl"] = Config::peerUrl;
    doc["wifiSSID"] = Config::wifiSSID;
    doc["debug"] = Config::debug;
    doc["polling"] = Config::polling;

    std::string json;
    serializeJson(doc, json);
    return json;
}

double Config::get_key_ttl()
{
    return keyExpireTime - static_cast<double>(time(nullptr));
}
