#pragma once

#include <string>

// Initialize the network interface for the current build target.
//
// QEMU build (CONFIG_ETH_USE_OPENETH=y):
//   Brings up the OpenCores Ethernet interface emulated by QEMU and starts
//   the DHCP client.  ssid and password are ignored.
//   QEMU passes  -nic user,model=open_eth  by default, giving the VM
//   SLIRP user-mode NAT to the host network.  emulate.sh adds port
//   forwarding so that the host can initiate connections to the agent.
//
// Hardware build (CONFIG_ETH_USE_OPENETH not set):
//   Initialises the WiFi stack in station mode and connects to the AP
//   identified by ssid if ssid is non-empty.  Does nothing if ssid is
//   empty (credentials not yet configured; use the serial TUI to set them).
void network_init(const std::string& ssid, const std::string& password);

// Connect or reconnect WiFi with the given credentials.
// On a QEMU build this is a no-op.
void network_wifi_connect(const std::string& ssid, const std::string& password);
