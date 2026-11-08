#include "agent.hpp"
#include "config.hpp"
#include "control.hpp"
#include "network.hpp"
#include "state.hpp"
#include "time.hpp"

#include <ArduinoJson.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

TaskHandle_t crypto_task_handle;

static AgentClient& client = shared_agent_client();
static Config& config = shared_config();

static network_message::ExchangeKeys current_key_exchange()
{
    network_message::ExchangeKeys exchange;
    exchange.src = config.agtuuid;
    exchange.timestamp = get_current_time();
    return exchange;
}

void crypto_task(void* p)
{
    ESP_LOGI("Crypto", "Crypto task started");

    for (;;)
    {
        client.set_url(config.peerUrl);
        client.set_agtuuid(config.agtuuid);
        client.set_keyuuid(config.keyuuid);
        client.set_key(config.key);
        vTaskDelay(config.crypto_poll_ticks);

        if (!config.polling)
            continue;

        std::string exchange_response_json_str =
            client.send_network_message(current_key_exchange().to_json());

        if (exchange_response_json_str.empty())
        {
            ESP_LOGW("Crypto", "Empty exchange response");
            continue;
        };

        if (network_message::network_message_type(exchange_response_json_str) !=
            network_message::NetworkMessageType::ExchangeKeys)
        {
            ESP_LOGW("Crypto", "Unexpected exchange response type",
                     exchange_response_json_str.c_str());
            continue;
        }

        auto exchange = network_message::ExchangeKeys::from_json(exchange_response_json_str);

        if (!exchange.best_key_uuid().has_value())
        {
            ESP_LOGW("Crypto", "No keys found in exchange response");
            continue;
        }

        if (exchange.best_key_uuid().value() == config.keyuuid)
        {
            ESP_LOGI("Crypto", "Already using current key");
            continue;
        }

        ESP_LOGI("Crypto", "Applying new key: %s", exchange.best_key_uuid().value().c_str());

        auto key = exchange.best_key().value();
        memcpy(config.key, key.data(), sizeof(config.key));

        strncpy(config.keyuuid, exchange.best_key_uuid().value().c_str(),
                sizeof(config.keyuuid) - 1);
        config.keyuuid[sizeof(config.keyuuid) - 1] = '\0';
        config.keyExpireTime = get_current_time() + exchange.best_key_ttl().value();
        config.keytype = KeyType::DIST;
        config.save();
    }
}

void start_crypto()
{
    const BaseType_t ok = xTaskCreatePinnedToCore(crypto_task, "Crypto", 5000, nullptr, 2,
                                                  &crypto_task_handle, tskNO_AFFINITY);
    ESP_ERROR_CHECK(ok == pdPASS ? ESP_OK : ESP_FAIL);
}
