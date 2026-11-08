#include "ethernet.hpp"
#include "state.hpp"
#include "time.hpp"

extern void start_console();
extern void start_processor();
extern void start_crypto();

extern "C" void app_main(void)
{
    // Load configuration from NVS.
    Config& config = shared_config();

    // Bring up the network interface.
    // QEMU build: OpenCores Ethernet + DHCP (ssid/password ignored).
    // Hardware build: WiFi STA using stored credentials.
    network_init(config.wifiSSID, config.wifiPassword);

    // Start SNTP to get the current time, which is used for ticket trace responses.
    start_sntp();

    // Start the serial configuration TUI in its own FreeRTOS task.
    // Returns immediately; the REPL runs in the background.
    start_console();

    // Start the crypto task.
    start_crypto();

    // Start the processor in its own FreeRTOS task.
    // Returns immediately; the processor runs in the background.
    start_processor();
}