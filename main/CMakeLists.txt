idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES helloworld
    REQUIRES kvstore
    REQUIRES executor
)
