#pragma once

#include "kvstore.h"

#include <cstdint>
#include <string>

class Config
{
  public:
    Config();
    ~Config();

    // Persist all current field values back to NVS.
    void save();
    void set_passphrase(const std::string& passphrase);

    char        agtuuid[36];
    uint8_t     key[32];
    std::string peerUrl;
    std::string wifiSSID;
    std::string wifiPassword;

  private:
    KVStore _kvstore;
};

/*

Can you implement a simple TUI that is accessed via serial port?
Implement it in main/configure.cpp.
This is TUI is for configuring the agent after the binary has been flashed onto a chipset.

If possible on the ESP32, run the TUI in a separate thread so that it can be accessed at any time
without blocking the main agent process. If there is a managed component from arduino for handling
serial i/o, bring it in. If there is a managed component for handling wifi, bring it in and
integrate it into the TUI for managing wifi credentials.

The TUI needs to be able to do the following workflows:
list and show settings in a #sym:Config class instance
set individual settings in a #sym:Config class instance

Stub out commands using the following control forms:

Self::CreatePeer(_)   => "create_peer"
Self::DiscoverPeer(_) => "discover_peer"
Self::DeletePeers(_)  => "delete_peers"
Self::GetPeers(_)     => "get_peers"
Self::GetRoutes(_)    => "get_routes"
Self::SyncProcess(_)  => "sync_process"
Self::WriteFile(_)    => "write_file"
Self::LoadFile(_)     => "load_file"
Self::GetConfig(_)    => "get_config"

Look at ../stembot-rust for reference

*/