idf_component_register(
    SRCS "main.cpp" "configure.cpp"
    INCLUDE_DIRS "."
    REQUIRES helloworld
    REQUIRES kvstore
    REQUIRES executor
    REQUIRES models
)
