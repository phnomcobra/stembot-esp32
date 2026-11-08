idf_component_register(
    SRCS "config.cpp" "control.cpp" "crypto.cpp" "network.cpp" "utils.cpp"
    INCLUDE_DIRS "include"
    REQUIRES kvstore mbedtls idf::bblanchon__arduinojson
)
