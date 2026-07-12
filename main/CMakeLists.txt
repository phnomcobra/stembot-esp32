idf_component_register(
    SRCS "main.cpp" "configure.cpp" "network.cpp" "processor.cpp"
    INCLUDE_DIRS "."
    REQUIRES kvstore
    REQUIRES executor
    REQUIRES models
    REQUIRES console
    REQUIRES esp_wifi
    REQUIRES esp_event
    REQUIRES esp_netif
    REQUIRES esp_eth
)
