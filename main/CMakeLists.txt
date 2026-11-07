idf_component_register(
    SRCS "crypto.cpp" "main.cpp" "configure.cpp" "processor.cpp"
    INCLUDE_DIRS "."
    REQUIRES kvstore
    REQUIRES executor
    REQUIRES models
    REQUIRES console
    REQUIRES service
)
