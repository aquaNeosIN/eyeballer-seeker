#include "camera_feed.h"

#include "config.h"
#include "track.h"

#include "esp_camera.h"
#include "esp_log.h"
#include "esp_timer.h"

#include <math.h>

static const char *TAG = "camera";

static int s_started;
static track_state_t s_track;
static uint8_t s_prev[TRACK_MAX_PIXELS];
static uint8_t s_luma[TRACK_MAX_PIXELS];
static int s_logged_format;

#if SIMULATE_CAMERA
static void simulate(sighting_t *out)
{
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    float phase = (float)(now % 5000u) / 5000.0f;
    if (phase < 0.12f || phase > 0.88f) {
        out->present = false;
        out->dropped_close = false;
        out->x = 0;
        out->y = 0;
        out->area = 0;
        return;
    }
    float t = (phase - 0.12f) / 0.76f;
    out->present = true;
    out->dropped_close = false;
    out->x = -cosf(t * 3.14159265f);
    out->y = 0.15f * sinf(t * 6.2831853f);
    out->area = 0.08f;
}
#endif

bool camera_feed_start(void)
{
    track_bind(&s_track, s_prev, TRACK_MAX_PIXELS);
#if SIMULATE_CAMERA
    s_started = 1;
    ESP_LOGW(TAG, "SIMULATE_CAMERA=1, sensor is not opened");
    return true;
#else
    camera_config_t config = {
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,
        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,
        .xclk_freq_hz = CAM_XCLK_HZ,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_RGB565,
        .frame_size = CAM_FRAME_QVGA ? FRAMESIZE_QVGA : FRAMESIZE_QQVGA,
        .jpeg_quality = 12,
        .fb_count = 2,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_LATEST,
        .sccb_i2c_port = -1,
        .jpeg_buffer_size = 0,
    };
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_camera_init: %s", esp_err_to_name(err));
        return false;
    }
    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor) {
        ESP_LOGI(TAG, "sensor pid=0x%04x %s", sensor->id.PID,
                 sensor->id.PID == 0x5640 ? "OV5640" : "not OV5640");
        if (CAM_VFLIP && sensor->set_vflip) {
            sensor->set_vflip(sensor, 1);
        }
        if (CAM_HMIRROR && sensor->set_hmirror) {
            sensor->set_hmirror(sensor, 1);
        }
    }
    s_started = 1;
    ESP_LOGI(TAG, "DVP RGB565 %s  SCCB %d/%d  PCLK %d", CAM_FRAME_QVGA ? "QVGA" : "QQVGA",
             CAM_PIN_SIOC, CAM_PIN_SIOD, CAM_PIN_PCLK);
    return true;
#endif
}

void camera_feed_poll(sighting_t *out)
{
    *out = (sighting_t){0};
    if (!s_started) {
        return;
    }
#if SIMULATE_CAMERA
    simulate(out);
    return;
#else
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGW(TAG, "no frame");
        return;
    }
    if (fb->format != PIXFORMAT_RGB565) {
        if (!s_logged_format) {
            ESP_LOGW(TAG, "frame format %d is not RGB565", (int)fb->format);
            s_logged_format = 1;
        }
        esp_camera_fb_return(fb);
        return;
    }
    int step = (int)fb->width / 80;
    if (step < 1) {
        step = 1;
    }
    while (step < 16 && ((int)fb->width / step) * ((int)fb->height / step) > TRACK_MAX_PIXELS) {
        step++;
    }
    int lw = 0;
    int lh = 0;
    if (!track_rgb565_to_luma((const uint16_t *)fb->buf, (int)fb->width, (int)fb->height, step,
                              s_luma, TRACK_MAX_PIXELS, &lw, &lh)) {
        esp_camera_fb_return(fb);
        return;
    }
    *out = track_motion(&s_track, s_luma, lw, lh);
    esp_camera_fb_return(fb);
#endif
}
