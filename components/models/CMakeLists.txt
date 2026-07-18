idf_component_register(
    SRCS "config.cpp" "control.cpp" "network.cpp" "utils.cpp"
    INCLUDE_DIRS "include"
    REQUIRES kvstore mbedtls idf::bblanchon__arduinojson
)
