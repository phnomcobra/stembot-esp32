#pragma once

#include <cstdint>
#include <string>

#include "kvstore.h"

class Config
{
  public:
    Config();
    ~Config();

    char    agtuuid[36];
    uint8_t key[32];

  private:
    KVStore _kvstore;
};