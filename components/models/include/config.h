#pragma once

#include "kvstore.h"

#include <cstdint>
#include <string>

class Config
{
  public:
    Config();
    ~Config();

    char agtuuid[36];
    uint8_t key[32];

  private:
    KVStore _kvstore;
};