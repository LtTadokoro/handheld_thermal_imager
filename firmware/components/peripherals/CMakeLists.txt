idf_component_register(
    SRCS "adc.c" "gpio.c" "nvs_wr.c" "SD_card.c"  INCLUDE_DIRS "include"
    PRIV_REQUIRES freertos log esp_driver_gpio esp_driver_ledc esp_adc nvs_flash fatfs sdmmc esp_driver_spi display mlx90640 task
)