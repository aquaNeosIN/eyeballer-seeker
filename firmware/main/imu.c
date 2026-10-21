#include "imu.h"

#include "config.h"
#include "i2c_bus.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <math.h>

static const char *TAG = "imu";

#define QMI_WHOAMI 0x00
#define QMI_WHOAMI_VALUE 0x05
#define QMI_CTRL1 0x02
#define QMI_CTRL2 0x03
#define QMI_CTRL3 0x04
#define QMI_CTRL7 0x08
#define QMI_CTRL8 0x09
#define QMI_RESET 0x60
#define QMI_RST_RESULT 0x4D
#define QMI_AX_L 0x35

#define QMI_ACC_RANGE_4G 1
#define QMI_ACC_ODR_250 5
#define QMI_GYR_RANGE_512 5
#define QMI_GYR_ODR_224 5

static bool s_ready;

static bool write_reg(uint8_t reg, uint8_t value)
{
    return board_i2c_write(QMI8658_ADDR, reg, &value, 1) == ESP_OK;
}

bool imu_start(void)
{
    s_ready = false;
    if (!write_reg(QMI_RESET, 0xB0)) {
        ESP_LOGW(TAG, "QMI8658 reset write failed");
        return false;
    }
    bool reset_ok = false;
    for (int i = 0; i < 20; ++i) {
        vTaskDelay(pdMS_TO_TICKS(10));
        uint8_t result = 0;
        if (board_i2c_read(QMI8658_ADDR, QMI_RST_RESULT, &result, 1) == ESP_OK && result == 0x80) {
            reset_ok = true;
            break;
        }
    }
    if (!reset_ok) {
        ESP_LOGW(TAG, "QMI8658 reset did not report 0x80");
        return false;
    }
    // SensorLib sets CTRL1 bit 6 (address auto-increment) after reset.
    uint8_t ctrl1 = 0;
    if (board_i2c_read(QMI8658_ADDR, QMI_CTRL1, &ctrl1, 1) != ESP_OK) {
        return false;
    }
    ctrl1 |= 0x40;
    if (!write_reg(QMI_CTRL1, ctrl1)) {
        return false;
    }
    write_reg(QMI_CTRL8, 0x80);

    uint8_t who = 0;
    if (board_i2c_read(QMI8658_ADDR, QMI_WHOAMI, &who, 1) != ESP_OK || who != QMI_WHOAMI_VALUE) {
        ESP_LOGW(TAG, "QMI8658 WHO_AM_I 0x%02X", who);
        return false;
    }

    write_reg(QMI_CTRL7, 0x00);
    // CTRL2: range in bits 6:4, ODR in bits 3:0. Same packing as SensorLib.
    write_reg(QMI_CTRL2, (uint8_t)((QMI_ACC_RANGE_4G << 4) | QMI_ACC_ODR_250));
    write_reg(QMI_CTRL3, (uint8_t)((QMI_GYR_RANGE_512 << 4) | QMI_GYR_ODR_224));
    if (!write_reg(QMI_CTRL7, 0x03)) {
        return false;
    }
    s_ready = true;
    ESP_LOGI(TAG, "QMI8658 online, WHO_AM_I 0x%02X", who);
    return true;
}

bool imu_ready(void)
{
    return s_ready;
}

bool imu_read(imu_sample_t *out)
{
    if (!s_ready) {
        return false;
    }
    uint8_t raw[12];
    if (board_i2c_read(QMI8658_ADDR, QMI_AX_L, raw, sizeof(raw)) != ESP_OK) {
        return false;
    }
    int16_t axis[6];
    for (int i = 0; i < 6; ++i) {
        axis[i] = (int16_t)((uint16_t)raw[i * 2] | ((uint16_t)raw[i * 2 + 1] << 8));
    }
    const float acc_scale = 4.0f / 32768.0f;
    const float gyr_scale = 512.0f / 32768.0f;
    out->ax_g = axis[0] * acc_scale;
    out->ay_g = axis[1] * acc_scale;
    out->az_g = axis[2] * acc_scale;
    out->gx_dps = axis[3] * gyr_scale;
    out->gy_dps = axis[4] * gyr_scale;
    out->gz_dps = axis[5] * gyr_scale;
    return true;
}

bool imu_wrist_moving(const imu_sample_t *sample)
{
    static bool have_base;
    static float bx, by, bz;
    if (!have_base) {
        bx = sample->ax_g;
        by = sample->ay_g;
        bz = sample->az_g;
        have_base = true;
        return false;
    }
    float dx = sample->ax_g - bx;
    float dy = sample->ay_g - by;
    float dz = sample->az_g - bz;
    bx += 0.02f * dx;
    by += 0.02f * dy;
    bz += 0.02f * dz;
    float acc = sqrtf(dx * dx + dy * dy + dz * dz);
    float gyro = sqrtf(sample->gx_dps * sample->gx_dps + sample->gy_dps * sample->gy_dps +
                       sample->gz_dps * sample->gz_dps);
    return acc > ACCEL_MOVE_G || gyro > GYRO_MOVE_DPS;
}
