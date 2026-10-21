idf_component_register(
    SRCS
        "app_main.c"
        "ld2450.c"
        "gaze.c"
        "face.c"
        "i2c_bus.c"
        "pmu.c"
        "imu.c"
        "display.c"
        "ipc.c"
        "tasks.c"
    INCLUDE_DIRS "."
    REQUIRES esp_lcd esp_lcd_co5300 driver esp_timer
)
