// ethernet.cpp
// Brings up the correct network interface depending on the build target.
//
//   QEMU  (CONFIG_ETH_USE_OPENETH=y)  — OpenCores Ethernet + DHCP
//   ESP32 (default)                   — WiFi STA with stored credentials
//
// network_init() is called once from app_main before event_loop().

#include "ethernet.hpp"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"

static const char* TAG = "ethernet";

// ═════════════════════════════════════════════════════════════════════════════
#if CONFIG_ETH_USE_OPENETH
// ── QEMU path: OpenCores Ethernet + DHCP ─────────────────────────────────────
// QEMU emulates the OpenCores Ethernet IP via  -nic user,model=open_eth.
// SLIRP user-mode networking gives the VM full outbound access to the host
// network.  emulate.sh adds  hostfwd  rules so the host can also reach the
// agent's HTTP listener.
// ═════════════════════════════════════════════════════════════════════════════

#include "esp_eth.h"
#include "esp_eth_mac_openeth.h"
#include "esp_eth_phy.h"

static void on_eth_event(void* /*arg*/, esp_event_base_t /*base*/, int32_t event_id, void* /*data*/)
{
    switch (event_id)
    {
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet started");
        break;
    case ETHERNET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Ethernet link up");
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet link down");
        break;
    default:
        break;
    }
}

#include "time.hpp"

static void on_eth_got_ip(void* /*arg*/, esp_event_base_t /*base*/, int32_t event_id, void* data)
{
    if (event_id == IP_EVENT_ETH_GOT_IP)
    {
        const auto* ev = static_cast<ip_event_got_ip_t*>(data);
        ESP_LOGI(TAG, "Ethernet got IP: " IPSTR, IP2STR(&ev->ip_info.ip));
        start_sntp();
    }
}

void network_init(const std::string& /*ssid*/, const std::string& /*password*/)
{
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());

    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t* eth_netif = esp_netif_new(&netif_cfg);

    eth_mac_config_t mac_cfg = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_cfg = ETH_PHY_DEFAULT_CONFIG();
    // OpenCores Ethernet in QEMU has no real PHY; skip autonegotiation
    // and hardware reset.
    phy_cfg.autonego_timeout_ms = 100;
    phy_cfg.reset_gpio_num = -1;

    esp_eth_mac_t* mac = esp_eth_mac_new_openeth(&mac_cfg);
    esp_eth_phy_t* phy = esp_eth_phy_new_generic(&phy_cfg);

    esp_eth_config_t eth_cfg = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = nullptr;
    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_cfg, &eth_handle));
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));

    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, on_eth_event, nullptr));
    ESP_ERROR_CHECK(
        esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, on_eth_got_ip, nullptr));

    ESP_ERROR_CHECK(esp_eth_start(eth_handle));
}

void network_wifi_connect(const std::string& /*ssid*/, const std::string& /*password*/)
{
    // No-op: QEMU uses Ethernet, not WiFi.
}

// ═════════════════════════════════════════════════════════════════════════════
#else
// ── Hardware path: WiFi STA ───────────────────────────────────────────────────
// ═════════════════════════════════════════════════════════════════════════════

#include "esp_wifi.h"

static bool s_wifi_started = false;

static void on_wifi_event(void* /*arg*/, esp_event_base_t /*base*/, int32_t event_id,
                          void* /*data*/)
{
    if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "WiFi disconnected");
    }
}

#include "time.hpp"

static void on_wifi_got_ip(void* /*arg*/, esp_event_base_t /*base*/, int32_t event_id, void* data)
{
    if (event_id == IP_EVENT_STA_GOT_IP)
    {
        const auto* ev = static_cast<ip_event_got_ip_t*>(data);
        ESP_LOGI(TAG, "WiFi got IP: " IPSTR, IP2STR(&ev->ip_info.ip));
        start_sntp();
    }
}

void network_init(const std::string& ssid, const std::string& password)
{
    if (s_wifi_started)
        return;

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_wifi_event,
                                                        nullptr, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        on_wifi_got_ip, nullptr, nullptr));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    s_wifi_started = true;

    if (!ssid.empty())
    {
        network_wifi_connect(ssid, password);
    }
    else
    {
        ESP_LOGI(TAG, "WiFi credentials not set — use the serial TUI to configure them");
    }
}

void network_wifi_connect(const std::string& ssid, const std::string& password)
{
    if (!s_wifi_started || ssid.empty())
        return;

    wifi_config_t wifi_cfg = {};
    strncpy(reinterpret_cast<char*>(wifi_cfg.sta.ssid), ssid.c_str(),
            sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy(reinterpret_cast<char*>(wifi_cfg.sta.password), password.c_str(),
            sizeof(wifi_cfg.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    esp_wifi_connect();
    ESP_LOGI(TAG, "Connecting to WiFi SSID '%s'...", ssid.c_str());
}

#endif // CONFIG_ETH_USE_OPENETH
