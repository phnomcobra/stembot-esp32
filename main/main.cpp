#include "helloworld.h"
#include "esp_log.h"

static const char *TAG = "stembot";

extern "C" void app_main(void)
{
    const std::string message = helloworld::greet("World");
    ESP_LOGI(TAG, "%s", message.c_str());
}
