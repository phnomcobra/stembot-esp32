idf_component_register(
    SRCS "main.cpp" "configure.cpp" "processor.cpp"
    INCLUDE_DIRS "."
    REQUIRES kvstore
    REQUIRES executor
    REQUIRES models
    REQUIRES console
    REQUIRES service
)
