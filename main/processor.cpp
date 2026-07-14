#include "agent.hpp"
#include "config.hpp"
#include "control.hpp"
#include "network.hpp"

#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

TaskHandle_t processor_task_handle;

void processor_task(void* p)
{
    ESP_LOGI("Processor", "Processor task started");

    for (;;)
    {
        // Task code goes here
        Config config;

        network_message::NetworkMessagesRequest messages_request;
        messages_request.src = config.agtuuid;
        messages_request.timestamp = static_cast<double>(esp_timer_get_time()) / 1e6;
        messages_request.limit = 1;

        AgentClient agent_client(config.peerUrl, config.key, config.agtuuid);

        std::string messages_response_json = agent_client.send_network_message(messages_request.to_json());
        if (messages_response_json.empty())
        {
            ESP_LOGE("Processor", "Failed to receive response");
            vTaskDelay(10000 / portTICK_PERIOD_MS);
            continue;
        }

        auto messages_response = network_message::NetworkMessagesResponse::from_json(messages_response_json);

        ESP_LOGI("Processor", "Received %zu messages", messages_response.message_jsons.size());

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void start_processor()
{
    ESP_ERROR_CHECK(!xTaskCreatePinnedToCore(processor_task, "Processor", 5000, NULL, 2,
                                             &processor_task_handle, tskNO_AFFINITY));
}
