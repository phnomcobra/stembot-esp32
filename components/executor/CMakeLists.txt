idf_component_register(
    SRCS "agent/aes_eax.cpp" "agent/agent.cpp"
    INCLUDE_DIRS "include" "agent/include"
    REQUIRES esp_http_client mbedtls models
)
