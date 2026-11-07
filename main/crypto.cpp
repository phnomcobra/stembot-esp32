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
#include <mbedtls/base64.h>

TaskHandle_t crypto_task_handle;
const TickType_t poll_ticks = 60000 / portTICK_PERIOD_MS;

static std::string base64_key(const uint8_t key[32])
{
    unsigned char encoded[45] = {};
    size_t length = 0;
    if (mbedtls_base64_encode(encoded, sizeof(encoded), &length, key, 32) != 0)
        return {};
    return {reinterpret_cast<char*>(encoded), length};
}

static bool apply_key_distribution(const network_message::KeyDistribution& distribution)
{
    uint8_t key[32] = {};
    size_t length = 0;
    if (mbedtls_base64_decode(key, sizeof(key), &length,
                              reinterpret_cast<const unsigned char*>(distribution.b64_key.data()),
                              distribution.b64_key.size()) != 0 ||
        length != sizeof(key) || distribution.objuuid.empty() || distribution.ttl <= 0.0)
    {
        return false;
    }

    /*
    memcpy(config.key, key, sizeof(config.key));
    strncpy(config.keyuuid, distribution.objuuid.c_str(), sizeof(config.keyuuid) - 1);
    config.keyuuid[sizeof(config.keyuuid) - 1] = '\0';
    config.user_key = false;
    config.key_ttl = distribution.ttl;
    config.save();
    client.set_key(config.key);
    client.set_keyuuid(config.keyuuid);
    */
    return true;
}

static network_message::ExchangeKeys current_key_exchange()
{
    network_message::ExchangeKeys exchange;
    exchange.src = config.agtuuid;
    exchange.timestamp = get_current_time();
    if (!config.user_key && config.key_ttl > 0.0)
    {
        exchange.distributions.push_back({base64_key(config.key), config.keyuuid, config.key_ttl});
    }
    return exchange;
}

void crypto_task(void* p)
{
    ESP_LOGI("Crypto", "Crypto task started");
    bool idle = true;

    for (;;)
    {
        if (idle)
        {
            config.load();
            client.set_url(config.peerUrl);
            client.set_agtuuid(config.agtuuid);
            client.set_keyuuid(config.keyuuid);
            client.set_key(config.key);
            vTaskDelay(poll_ticks);
        }

        if (!config.polling)
        {
            idle = true;
            continue;
        }

        std::string exchange_response =
            client.send_network_message(current_key_exchange().to_json());
        if (!exchange_response.empty() &&
            network_message::network_message_type(exchange_response) ==
                network_message::NetworkMessageType::ExchangeKeys)
        {
            process_network_message(exchange_response, false);
        }

    }
}

void start_crypto()
{
    const BaseType_t ok = xTaskCreatePinnedToCore(crypto_task, "Crypto", 5000, nullptr, 2,
                                                  &crypto_task_handle, tskNO_AFFINITY);
    ESP_ERROR_CHECK(ok == pdPASS ? ESP_OK : ESP_FAIL);
}
