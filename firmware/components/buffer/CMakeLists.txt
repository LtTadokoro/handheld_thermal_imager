idf_component_register(
    SRCS "buffer.c"    INCLUDE_DIRS "include"
    REQUIRES freertos display
    PRIV_REQUIRES esp_psram heap
)