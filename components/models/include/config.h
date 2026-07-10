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
    std::string peerUrl;

  private:
    KVStore _kvstore;
};

/*

Can you implement a simple TUI that is accessed via serial port? Implement it in main/configure.cpp.
This is TUI is for configuring the agent after the binary has been flashed onto a chipset.
The TUI needs to be able to list and show settings in a #sym:Config class instance.
The TUI also needs to be able to set individual settings in a #sym:Config class instance.
If there is a managed component from arduino for handling serial i/o, bring it in.

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