#include "agent.hpp"
#include "config.hpp"
#include "control.hpp"
#include "network.hpp"
#include "time.hpp"

#include <ArduinoJson.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

TaskHandle_t processor_task_handle;
AgentClient client;
Config config;

std::string process_control_form(const std::string& form_json)
{
    control_form::ControlFormType type = control_form::control_form_type(form_json);
    switch (type) {
        case control_form::ControlFormType::GetConfig:
        {
            auto form = control_form::GetConfig::from_json(form_json);
            if (config.debug) ESP_LOGI("Processor", "Processing GetConfig form");
            form.config_json = Config().to_json();
            return form.to_json();
        }
        case control_form::ControlFormType::Benchmark:
        {
            auto form = control_form::Benchmark::from_json(form_json);
            if (config.debug) ESP_LOGI("Processor", "Processing Benchmark form");
            if (form.inbound_size.has_value()) form.payload = std::string(*form.inbound_size, '0');
            else form.payload = std::nullopt;

            // Add processing logic here
            return form.to_json();
        }
        default:
        {
            if (config.debug)
            {
                JsonDocument doc;
                deserializeJson(doc, form_json);
                doc["error"] = "Unknown control form type";
                ESP_LOGW("Processor", "Unknown control form type: %s", doc["type"] | "(none)");
            }
            return form_json; // Echo back unrecognized forms
        }
    }
}


void process_network_message(const std::string& msg_json)
{
    network_message::NetworkMessageType type = network_message::network_message_type(msg_json);

    switch (type) {
        case network_message::NetworkMessageType::TicketRequest:
        {
            if (config.debug) ESP_LOGI("Processor", "Processing ticket request network message");

            auto ticket = network_message::NetworkTicket::from_json(msg_json);

            if(ticket.tracing) {
                network_message::TicketTraceResponse ticket_trace_response;
                ticket_trace_response.tckuuid = ticket.tckuuid;
                ticket_trace_response.network_ticket_type = "ticket_request";
                ticket_trace_response.hop_time = get_current_time();
                ticket_trace_response.src = config.agtuuid;
                ticket_trace_response.dest = ticket.src;
                client.send_network_message(ticket_trace_response.to_json());
            }

            ticket.form_json = process_control_form(ticket.form_json);

            if(ticket.tracing) {
                network_message::TicketTraceResponse trace_response;
                trace_response.tckuuid = ticket.tckuuid;
                trace_response.network_ticket_type = "ticket_response";
                trace_response.hop_time = get_current_time();
                trace_response.src = config.agtuuid;
                trace_response.dest = ticket.src;
                client.send_network_message(trace_response.to_json());
            }

            // invert src and dest for the response
            // turns ticket request into a ticket response
            ticket.dest = ticket.src;
            ticket.src = config.agtuuid;
            auto msg_json = ticket.to_json("ticket_response");

            client.send_network_message(msg_json);

            break;
        }
        case network_message::NetworkMessageType::Ping:
        {
            if (config.debug) ESP_LOGI("Processor", "Processing ping network message");

            auto ping = network_message::Ping::from_json(msg_json);

            network_message::Acknowledgement ack;
            ack.ack_type = "ping";
            ack.src = config.agtuuid;
            ack.dest = ping.src;
            ack.timestamp = get_current_time();
            client.send_network_message(ack.to_json());

            break;
        }
        default:
        {
            JsonDocument doc;
            deserializeJson(doc, msg_json);
            if (config.debug) ESP_LOGW("Processor", "Unknown network message type: %s", doc["type"] | "(none)");
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
        if (idle) {
            config.load();
            client.set_url(config.peerUrl);
            client.set_agtuuid(config.agtuuid);
            client.set_key(config.key);
            vTaskDelay(config.poll_ticks);
        }

        if (!config.polling)
        {
            idle = true;
            continue;
        }

        network_message::NetworkMessagesRequest messages_request;
        messages_request.src = config.agtuuid;
        messages_request.timestamp = get_current_time();
        messages_request.limit = 1;

        std::string messages_response_json = client.send_network_message(messages_request.to_json());
        if (messages_response_json.empty())
        {
            if (config.debug) ESP_LOGE("Processor", "Failed to receive response");
            idle = true;
            continue;
        }

        auto messages_response = network_message::NetworkMessagesResponse::from_json(messages_response_json);
        idle = messages_response.message_jsons.empty();

        for (const std::string& msg_json : messages_response.message_jsons) process_network_message(msg_json);
    }
}

void start_processor()
{
    ESP_ERROR_CHECK(!xTaskCreatePinnedToCore(processor_task, "Processor", 5000, NULL, 2,
                                             &processor_task_handle, tskNO_AFFINITY));
}
