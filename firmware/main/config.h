#pragma once

// eyeballer-seeker v2. Change a value here, then rebuild.
// Sources for each group are named next to it. Integers only, so the
// host test can include this header without ESP-IDF.

// ---------------------------------------------------------------------------
// Board. Alientek DNESP32S3, module assumed ESP32-S3-WROOM-1-N16R8
// (16 MB quad flash, 8 MB octal PSRAM). See hardware/v2-dnesp32s3-camera/.
// ---------------------------------------------------------------------------

// SPI LCD, Alientek IDF guide V1.6 (camera / SPI LCD chapters):
//   MOSI GPIO11, SCK GPIO12, CS GPIO21, DC GPIO40.
// DC reaches the SPILCD socket only when P5 jumpers IO_SET to LCD_DC
// (some silkscreens print IO_SEL). A Dupont wire on the GPIO40 pad
// itself does not need the jumper.
#define LCD_MOSI_PIN 11
#define LCD_SCK_PIN 12
#define LCD_CS_PIN 21
#define LCD_DC_PIN 40
// SPI2 (FSPI). The numeric host id is SPI2_HOST from the IDF header,
// which is 1 on the ESP32-S3, so it is not hardcoded here.
#define LCD_SPI_HZ (20 * 1000 * 1000)

#define LCD_WIDTH 240
#define LCD_HEIGHT 280

// ST7789V2 RAM is 240x320. The 1.69" glass is 240x280, so portrait
// mode starts 20 rows down. Waveshare wiki and TFT_eSPI's 240x280
// setup both use this row offset. Column offset stays 0.
#define LCD_COL_OFFSET 0
#define LCD_ROW_OFFSET 20

// IPS panels of this module expect inverted colors. Set to 0 if the
// picture looks like a negative.
#ifndef LCD_INVERT_COLOR
#define LCD_INVERT_COLOR 1
#endif

// The ST7789 driver is told the framebuffer is little-endian RGB565,
// which is how this code stores uint16 pixels. That sets the panel
// RAMCTL endian bit. There is no separate software byte swap.

// 0 = RGB, 1 = BGR. Flip if red and blue are swapped.
#ifndef LCD_BGR
#define LCD_BGR 0
#endif

// MADCTL mirrors. Flip if the face is upside down or mirrored.
#ifndef LCD_MIRROR_X
#define LCD_MIRROR_X 0
#endif
#ifndef LCD_MIRROR_Y
#define LCD_MIRROR_Y 0
#endif

// 1: LCD reset (XL9555 IO1_2) and backlight (XL9555 IO1_3) follow the
//    board's SPILCD socket. This is the wiring in the v2 hardware notes.
// 0: drive LCD_RST_GPIO and LCD_BL_GPIO instead. See the hardware README.
#ifndef LCD_USE_XL9555
#define LCD_USE_XL9555 1
#endif

// Used only when LCD_USE_XL9555 is 0.
// GPIO8 is not used by the camera, codec, SD, or octal PSRAM.
// GPIO43 is UART0 TX on the extra serial header. Leave that header
// unconnected while it drives the backlight.
#define LCD_RST_GPIO 8
#define LCD_BL_GPIO 43

// Shared I2C to the XL9555. Alientek guide: SDA GPIO41, SCL GPIO42.
// ES8388 (0x10) and the 24C02 EEPROM share this bus. This firmware
// only addresses the expander.
#define I2C_SDA_PIN 41
#define I2C_SCL_PIN 42
#define I2C_HZ 400000

// XL9555, 7-bit address 0x20 (A2..A0 grounded). Bit numbers are the
// 16-bit port: 0..7 = IO0_0..IO0_7, 8..15 = IO1_0..IO1_7.
// Guide chapter 19 and the camera chapter:
//   beep IO0_3, camera PWDN IO0_4, camera RESET IO0_5,
//   LCD reset IO1_2, LCD backlight IO1_3,
//   keys IO1_4..IO1_7 (inputs, not used here).
#define XL9555_ADDR 0x20
#define XL_BEEP 3
#define XL_OV_PWDN 4
#define XL_OV_RESET 5
#define XL_LCD_RST 10
#define XL_LCD_BL 11

// The EXIO example turns the beeper on by writing 0 and off by writing 1.
#define XL_BEEP_OFF 1

// ---------------------------------------------------------------------------
// Camera. Alientek IDF guide V1.6 camera chapter, connector P2.
// RESET and PWDN are on the XL9555, so the ESP32 pins stay unconnected.
// The kit module has its own 24 MHz oscillator, so XCLK is unconnected.
// ---------------------------------------------------------------------------
#define CAM_PIN_PWDN (-1)
#define CAM_PIN_RESET (-1)
#define CAM_PIN_XCLK (-1)
#define CAM_PIN_SIOD 39
#define CAM_PIN_SIOC 38
#define CAM_PIN_D0 4
#define CAM_PIN_D1 5
#define CAM_PIN_D2 6
#define CAM_PIN_D3 7
#define CAM_PIN_D4 15
#define CAM_PIN_D5 16
#define CAM_PIN_D6 17
#define CAM_PIN_D7 18
#define CAM_PIN_VSYNC 47
#define CAM_PIN_HREF 48
#define CAM_PIN_PCLK 45
#define CAM_XCLK_HZ 24000000

// 1 = QVGA (320x240). 0 = QQVGA (160x120). Both are subsampled to
// about 80x60 before the motion pass.
#ifndef CAM_FRAME_QVGA
#define CAM_FRAME_QVGA 1
#endif

// Applied after the sensor probes, if the driver supports the calls.
#ifndef CAM_VFLIP
#define CAM_VFLIP 0
#endif
#ifndef CAM_HMIRROR
#define CAM_HMIRROR 0
#endif

// 1: skip the sensor and publish a synthetic walker.
// 0: read the OV5640. A build can pass -DSIMULATE_CAMERA=1.
#ifndef SIMULATE_CAMERA
#define SIMULATE_CAMERA 0
#endif

// ---------------------------------------------------------------------------
// Motion gate. One cluster, no face detector, no depth.
// Area is the fraction of sampled pixels that changed.
// A blob covering more than TRACK_MAX_AREA of the frame is dropped.
// That is this version's stand-in for "ignore the wearer": the camera
// looks outward, and a shape that fills the lens is too close.
// ---------------------------------------------------------------------------
#define TRACK_CELLS_X 10
#define TRACK_CELLS_Y 8
#define TRACK_MOTION_DELTA 28
#define TRACK_MIN_AREA 0.02f
#define TRACK_MAX_AREA 0.40f
#define TRACK_MAX_PIXELS (160 * 120)

// Gaze. Look values are -1..1 before FACE_MAX_PUPIL_PX.
// A larger moving blob (closer) is allowed a longer pupil travel.
#define TRACK_TAU_MS 90.0f
#define IDLE_TAU_MS 480.0f
#define LOST_LINGER_MS 1500
#define SIGHT_STALE_MS 300
#define AREA_GAIN_REF 0.10f
#define AREA_GAIN_FLOOR 0.35f
#define LOOK_Y_SCALE 0.45f
#define ANGLE_LIMIT_DEG 60.0f

#define FACE_MAX_PUPIL_PX 8

#define BLINK_PERIOD_MS 3200
#define BLINK_MS 160
#define GLANCE_PERIOD_MS 2800
#define GLANCE_HOLD_MS 420
#define GLANCE_PX 6.0f

#define DEBUG_PERIOD_MS 200
#define EYE_PERIOD_MS 33
#define CAM_PERIOD_MS 50
