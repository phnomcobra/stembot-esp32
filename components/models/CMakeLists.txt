idf_component_register(
    SRCS "config.cpp" "control.cpp" "key_derivation.cpp" "network.cpp" "utils.cpp"
    INCLUDE_DIRS "include"
    REQUIRES kvstore mbedtls idf::bblanchon__arduinojson
)
