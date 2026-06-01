#include "esp_log.h"
#include "helloworld.h"
#include "kvstore.h"

static const char* TAG = "stembot";

KVStore kvstore;

extern "C" void app_main(void)
{
    const std::string message = helloworld::greet("World");
    ESP_LOGI(TAG, "%s", message.c_str());
}
