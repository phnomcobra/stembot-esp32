#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "config.hpp"
#include <esp_log.h>

TaskHandle_t processor_task_handle;

void processor_task(void * p) {
    ESP_LOGI("Processor", "Processor task started");

    for (;;) {
        // Task code goes here
        Config config;

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


void start_processor()
{
    ESP_ERROR_CHECK(!xTaskCreatePinnedToCore(processor_task, "Processor", 5000, NULL, 2, &processor_task_handle, tskNO_AFFINITY));
}
