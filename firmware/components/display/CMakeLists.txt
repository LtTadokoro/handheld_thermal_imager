idf_component_register(
    SRCS "display.c" "image_processing.c" "ST7789_driver.c" INCLUDE_DIRS "include"
    REQUIRES esp_driver_gpio esp_common display buffer mlx90640
    PRIV_REQUIRES freertos log esp_timer esp_lcd esp_driver_spi task main
)