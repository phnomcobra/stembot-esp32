#pragma once

#include "kvstore.hpp"
#include "crypto.hpp"

#include <cstdint>
#include <freertos/FreeRTOS.h>
#include <string>

class Config
{
  public:
    Config();
    ~Config();

    void save();
    void load();
    void set_passphrase(const std::string& keyuuid, const std::string& passphrase);
    void set_user_key(const std::string& uuid, const std::string& passphrase);
    double get_key_ttl();

    std::string to_json() const;

    char agtuuid[36];
    char keyuuid[36];
    uint8_t key[32];
    KeyType keytype;
    double keyExpireTime;

    bool debug;
    bool polling;

    TickType_t processor_poll_ticks = 1000 / portTICK_PERIOD_MS;
    TickType_t crypto_poll_ticks = 60000 / portTICK_PERIOD_MS;

    std::string peerUrl;
    std::string wifiSSID;
    std::string wifiPassword;

  private:
    KVStore _kvstore;
};
