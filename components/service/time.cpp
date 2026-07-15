#include <esp_netif.h>
#include <esp_sntp.h>
#include <lwip/inet.h>
#include <time.h>

// Call once after Wi-Fi gets an IP:
void start_sntp()
{
    esp_netif_ip_info_t ip_info = {};
    esp_netif_t* netif = esp_netif_get_default_netif();
    if (netif == nullptr || esp_netif_get_ip_info(netif, &ip_info) != ESP_OK)
        return;

    // Format gateway IP as a string (e.g. "192.168.1.1")
    static char gw_str[16];
    esp_ip4addr_ntoa(&ip_info.gw, gw_str, sizeof(gw_str));

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, gw_str);
    esp_sntp_init();
}

double get_current_time() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec + tv.tv_usec / 1e6;
}
