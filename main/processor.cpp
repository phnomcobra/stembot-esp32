#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "config.hpp"

TaskHandle_t processor_task_handle;

void processor_task(void * p) {
    Config config;

    for (;;) {
        // Task code goes here
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


void start_processor()
{
    xTaskCreatePinnedToCore(processor_task, "Processor", 5000, NULL, 2, &processor_task_handle, tskNO_AFFINITY);
}
