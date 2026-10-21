#pragma once

// eyeballer-seeker v1. Edit a value here, then rebuild and flash.
// Pin sources are named next to each group.

// Radar UART. This repo's schematic (Waveshare PDF 2025-07-28):
//   H2 pin 7 GPIO17 = ESP32-S3 TX -> radar RX
//   H2 pin 8 GPIO18 = ESP32-S3 RX <- radar TX
// HARDWARE_REFERENCE.md lists the same GPIO numbers on header pins 6 and 7.
// The firmware uses the GPIO numbers. Meter the header before a 1:1 cable.
// 256000 8N1 is the LD2450 default in Hi-Link protocol PDF V1.03.
#define RADAR_TX_PIN 17
#define RADAR_RX_PIN 18
#define RADAR_BAUD 256000
#define RADAR_UART UART_NUM_1

// 1: a fake walker is fed through the real frame parser.
// 0: read the radar on UART1.
// A build can override this with -DSIMULATE_RADAR=0.
#ifndef SIMULATE_RADAR
#define SIMULATE_RADAR 1
#endif

// Display bus, from Waveshare Arduino libraries/Mylibrary/pin_config.h
// and the ESP-IDF BSP header esp32_s3_touch_amoled_1_75.h (same nets).
// https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75
#define LCD_SDIO0 4
#define LCD_SDIO1 5
#define LCD_SDIO2 6
#define LCD_SDIO3 7
#define LCD_SCLK 38
#define LCD_CS 12
#define LCD_RST 39
#define LCD_WIDTH 466
#define LCD_HEIGHT 466
// Column offset used by Waveshare 01_HelloWorld:
//   Arduino_CO5300(..., width, height, 6, 0, 0, 0)
// esp_lcd_co5300's draw path adds this gap when it rewrites CASET.
#define LCD_COL_OFFSET 6
// Brightness is a CO5300 command (0x51), in percent. There is no backlight GPIO.
#define LCD_BRIGHTNESS_PCT 63
#define LCD_SPI_HOST SPI2_HOST

// Shared I2C. pin_config.h IIC_SDA / IIC_SCL and HARDWARE_REFERENCE.md.
#define I2C_SDA 15
#define I2C_SCL 14
#define I2C_HZ 400000

// 7-bit addresses from HARDWARE_REFERENCE.md.
// QMI8658_L_SLAVE_ADDRESS in SensorLib is 0x6B. AXP2101 is 0x34.
#define QMI8658_ADDR 0x6B
#define AXP2101_ADDR 0x34

// LD2450 coordinates are millimetres. +x is right, +y is forward (protocol V1.03).
#define OWNER_MIN_MM 500
#define MIN_FORWARD_MM 200
#define MAX_RANGE_MM 6000
#define MOVING_SPEED_CM_S 10
#define TRACK_HYSTERESIS_MM 450
#define LOST_LINGER_MS 1500
#define TRACK_TAU_MS 90.0f
#define IDLE_TAU_MS 480.0f
#define ANGLE_LIMIT_DEG 60.0f
#define DIST_NEAR_MM 500.0f
#define DIST_FAR_MM 3000.0f
#define DIST_GAIN_FLOOR 0.30f

// Pupil travel at a full ±60 degree look, in pixels. face.h uses this.
#define FACE_MAX_PUPIL_PX 18

#define BLINK_PERIOD_MS 3200
#define BLINK_MS 160
#define GLANCE_PERIOD_MS 2800
#define GLANCE_HOLD_MS 420
#define GLANCE_PX 7.0f

// Motion gate. The register setup follows SensorLib, which the Waveshare
// QMI8658 Arduino example uses. These thresholds are for this firmware.
#define ACCEL_MOVE_G 0.12f
#define GYRO_MOVE_DPS 35.0f
#define MOTION_ENTER_MS 80
#define STILL_ENTER_MS 280

#define DEBUG_PERIOD_MS 200
#define EYE_PERIOD_MS 33
#define RADAR_STALE_MS 300
