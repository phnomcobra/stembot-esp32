#include "esp_log.h"
#include "helloworld.h"
#include "kvstore.h"

static const char* TAG = "stembot";

KVStore kvstore;

extern void event_loop();

extern "C" void app_main(void)
{
    const std::string message = helloworld::greet("World");
    ESP_LOGI(TAG, "%s", message.c_str());

    // Start the serial configuration TUI in its own FreeRTOS task.
    // Returns immediately; the REPL runs in the background.
    event_loop();
}
