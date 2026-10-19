idf_component_register(
    SRCS "menu.c" "ugui.c" "UI.c" INCLUDE_DIRS "include"
    PRIV_REQUIRES freertos buffer display mlx90640 peripherals task
)