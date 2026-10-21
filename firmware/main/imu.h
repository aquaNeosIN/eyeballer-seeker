#pragma once

#include <stdbool.h>

// QMI8658 bring-up follows SensorLib (reset, WHO_AM_I 0x05, ±4 g, ±512 dps),
// the same library the Waveshare 04_LVGL_QMI8658_ui example uses.
bool imu_start(void);
bool imu_ready(void);

typedef struct {
    float ax_g;
    float ay_g;
    float az_g;
    float gx_dps;
    float gy_dps;
    float gz_dps;
} imu_sample_t;

bool imu_read(imu_sample_t *out);
bool imu_wrist_moving(const imu_sample_t *sample);
