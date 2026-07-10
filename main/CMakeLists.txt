idf_component_register(
    SRCS "main.cpp" "configure.cpp"
    INCLUDE_DIRS "."
    REQUIRES helloworld
    REQUIRES kvstore
    REQUIRES executor
    REQUIRES models
    REQUIRES console
    REQUIRES esp_wifi
    REQUIRES esp_event
    REQUIRES esp_netif
)
