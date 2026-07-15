#pragma once

#include "kvstore.hpp"

#include <cstdint>
#include <string>

class Config
{
  public:
    Config();
    ~Config();

    void save();
    void set_passphrase(const std::string& passphrase);
    std::string to_json() const;

    char agtuuid[36];
    uint8_t key[32];
    bool debug;
    bool polling;
    std::string peerUrl;
    std::string wifiSSID;
    std::string wifiPassword;

  private:
    KVStore _kvstore;
};
