#include "config.hpp"
#include "esp_log.h"
#include "helloworld.hpp"
#include "network.hpp"

static const char* TAG = "stembot";

extern void event_loop();

extern "C" void app_main(void)
{
    const std::string message = helloworld::greet("World");
    ESP_LOGI(TAG, "%s", message.c_str());

    // Load configuration from NVS.
    Config config;

    // Bring up the network interface.
    // QEMU build: OpenCores Ethernet + DHCP (ssid/password ignored).
    // Hardware build: WiFi STA using stored credentials.
    network_init(config.wifiSSID, config.wifiPassword);

    // Start the serial configuration TUI in its own FreeRTOS task.
    // Returns immediately; the REPL runs in the background.
    event_loop();
}