idf_component_register(
    SRCS "config.cpp" "control.cpp"
    INCLUDE_DIRS "include"
    REQUIRES kvstore mbedtls
)
