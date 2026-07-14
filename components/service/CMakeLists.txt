idf_component_register(
    SRCS "ethernet.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi
    REQUIRES esp_event
    REQUIRES esp_netif
    REQUIRES esp_eth
    REQUIRES esp_timer
)
