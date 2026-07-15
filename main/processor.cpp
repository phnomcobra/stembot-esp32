#include "agent.hpp"
#include "config.hpp"
#include "control.hpp"
#include "network.hpp"
#include "time.hpp"

#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

TaskHandle_t processor_task_handle;

std::string process_control_form(const std::string& form_json)
{
    control_form::ControlFormType type = control_form::control_form_type(form_json);
    switch (type) {
    case control_form::ControlFormType::GetConfig:
    {
        auto form = control_form::GetConfig::from_json(form_json);
        ESP_LOGI("Processor", "Processing GetConfig form");
        form.config_json = Config().to_json();
        return form.to_json();
    }
    case control_form::ControlFormType::Benchmark:
    {
        auto form = control_form::Benchmark::from_json(form_json);
        ESP_LOGI("Processor", "Processing Benchmark form");
        if (form.inbound_size.has_value()) form.payload = std::string(*form.inbound_size, '0');
        else form.payload = std::nullopt;

        // Add processing logic here
        return form.to_json();
    }
    default:
        ESP_LOGW("Processor", "Unknown control form type");
        return form_json; // Echo back unrecognized forms
    }
}


void process_network_message(const std::string& msg_json)
{
    Config config;
    AgentClient agent_client(config.peerUrl, config.key, config.agtuuid);

    network_message::NetworkMessageType type = network_message::network_message_type(msg_json);

    switch (type) {
        case network_message::NetworkMessageType::TicketRequest:
        {
            auto ticket = network_message::NetworkTicket::from_json(msg_json);

            if(ticket.tracing) {
                network_message::TicketTraceResponse ticket_trace_response;
                ticket_trace_response.tckuuid = ticket.tckuuid;
                ticket_trace_response.network_ticket_type = "ticket_request";
                ticket_trace_response.hop_time = get_current_time();
                ticket_trace_response.src = config.agtuuid;
                ticket_trace_response.dest = ticket.src;
                agent_client.send_network_message(ticket_trace_response.to_json());
            }

            ticket.form_json = process_control_form(ticket.form_json);

            if(ticket.tracing) {
                network_message::TicketTraceResponse trace_response;
                trace_response.tckuuid = ticket.tckuuid;
                trace_response.network_ticket_type = "ticket_response";
                trace_response.hop_time = get_current_time();
                trace_response.src = config.agtuuid;
                trace_response.dest = ticket.src;
                agent_client.send_network_message(trace_response.to_json());
            }

            ticket.dest = ticket.src;
            ticket.src = config.agtuuid;

            auto msg_json = ticket.to_json("ticket_response");

            agent_client.send_network_message(msg_json);

            break;
        }
        default:
        {
            ESP_LOGW("Processor", "Unknown network message type");
            break;
        }
    }
}


void processor_task(void* p)
{
    ESP_LOGI("Processor", "Processor task started");
    bool idle = true;

    for (;;)
    {
        // Task code goes here
        Config config;

        if (!config.polling)
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        network_message::NetworkMessagesRequest messages_request;
        messages_request.src = config.agtuuid;
        messages_request.timestamp = static_cast<double>(esp_timer_get_time()) / 1e6;
        messages_request.limit = 1;

        AgentClient agent_client(config.peerUrl, config.key, config.agtuuid);

        std::string messages_response_json = agent_client.send_network_message(messages_request.to_json());
        if (messages_response_json.empty())
        {
            if (config.debug)
            {
                ESP_LOGE("Processor", "Failed to receive response");
            }
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        auto messages_response = network_message::NetworkMessagesResponse::from_json(messages_response_json);
        idle = messages_response.message_jsons.empty();

        for (const std::string& msg_json : messages_response.message_jsons) process_network_message(msg_json);

        if (config.debug)
        {
            ESP_LOGI("Processor", "Received %zu messages", messages_response.message_jsons.size());
        }

        if (idle) vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void start_processor()
{
    ESP_ERROR_CHECK(!xTaskCreatePinnedToCore(processor_task, "Processor", 5000, NULL, 2,
                                             &processor_task_handle, tskNO_AFFINITY));
}
