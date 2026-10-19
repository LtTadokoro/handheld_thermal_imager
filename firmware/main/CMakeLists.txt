idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES mlx90640 display task peripherals ui buffer vl53l1
)