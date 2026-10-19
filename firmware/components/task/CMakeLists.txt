idf_component_register(
    SRCS "task_management.c" INCLUDE_DIRS "include"
    REQUIRES freertos
    PRIV_REQUIRES log esp_timer buffer display mlx90640 peripherals ui vl53l1 main
)