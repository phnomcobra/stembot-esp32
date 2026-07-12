#include "config.hpp"
#include "esp_log.h"
#include "network.hpp"

extern void start_console();
extern void start_processor();

extern "C" void app_main(void)
{
    // Load configuration from NVS.
    Config config;

    // Bring up the network interface.
    // QEMU build: OpenCores Ethernet + DHCP (ssid/password ignored).
    // Hardware build: WiFi STA using stored credentials.
    network_init(config.wifiSSID, config.wifiPassword);

    // Start the serial configuration TUI in its own FreeRTOS task.
    // Returns immediately; the REPL runs in the background.
    start_console();

    // Start the processor in its own FreeRTOS task.
    // Returns immediately; the processor runs in the background.
    start_processor();
}