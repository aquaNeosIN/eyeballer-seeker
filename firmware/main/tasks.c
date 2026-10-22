#include "tasks.h"

#include "camera_feed.h"
#include "config.h"
#include "display.h"
#include "face.h"
#include "gaze.h"
#include "ipc.h"

#include "esp_heap_caps.h"
#include <stddef.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "tasks";

static uint32_t now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

void cam_task(void *arg)
{
    (void)arg;
    TickType_t wake = xTaskGetTickCount();
    while (true) {
        sight_msg_t msg = {
            .time_ms = now_ms(),
        };
        camera_feed_poll(&msg.sight);
        xQueueOverwrite(ipc_sight_queue(), &msg);
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(CAM_PERIOD_MS));
    }
}

void eye_task(void *arg)
{
    (void)arg;
    gaze_state_t gaze;
    gaze_reset(&gaze);
    sight_msg_t sight = {0};
    TickType_t wake = xTaskGetTickCount();
    uint32_t last_log = 0;

    while (true) {
        sight_msg_t incoming;
        if (xQueueReceive(ipc_sight_queue(), &incoming, 0) == pdTRUE) {
            sight = incoming;
        }
        uint32_t t = now_ms();
        sighting_t sample = sight.sight;
        if (sight.time_ms == 0 || t - sight.time_ms > SIGHT_STALE_MS) {
            sample.present = false;
        }
        gaze_output_t look = gaze_update(&gaze, &sample, t);
        eye_msg_t msg = {
            .gaze = look,
            .sight = sample,
        };
        xQueueOverwrite(ipc_eye_queue(), &msg);

        if (t - last_log >= DEBUG_PERIOD_MS) {
            last_log = t;
            const char *mode = look.tracking ? "track" : (look.lingering ? "linger" : "idle");
            ESP_LOGI(TAG, "%s x=%d y=%d area=%d look=%d,%d%s", mode, (int)(sample.x * 100),
                     (int)(sample.y * 100), (int)(sample.area * 1000), (int)(look.look_x * 100),
                     (int)(look.look_y * 100), sample.dropped_close ? " close" : "");
        }
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(EYE_PERIOD_MS));
    }
}

void display_task(void *arg)
{
    (void)arg;
    if (!display_ready()) {
        ESP_LOGW(TAG, "panel not started, display task idle");
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    const size_t pixels = (size_t)LCD_WIDTH * (size_t)LCD_HEIGHT;
    const size_t fb_bytes = (pixels * sizeof(uint16_t) + 63u) & ~63u;
    uint16_t *fb[2] = {NULL, NULL};
    int buffers = 0;
    for (int i = 0; i < 2; ++i) {
        fb[i] = heap_caps_aligned_alloc(64, fb_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!fb[i]) {
            break;
        }
        face_clear(fb[i], LCD_WIDTH, LCD_HEIGHT, k_face.bg);
        buffers++;
    }
    if (buffers == 0) {
        ESP_LOGE(TAG, "framebuffer alloc failed");
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    if (buffers == 1) {
        ESP_LOGW(TAG, "single framebuffer, %u bytes", (unsigned)(pixels * sizeof(uint16_t)));
        fb[1] = fb[0];
    } else {
        ESP_LOGI(TAG, "double framebuffer, %u bytes each", (unsigned)(pixels * sizeof(uint16_t)));
    }

    display_wait_idle();
    display_blit(0, 0, LCD_WIDTH, LCD_HEIGHT, fb[0]);

    int draw = buffers == 2 ? 1 : 0;
    eye_msg_t msg = {0};
    uint32_t frames = 0;
    uint32_t fps_mark = now_ms();
    const int band_top = k_face.band_top;
    const int band_h = k_face.band_bottom - band_top;

    while (true) {
        xQueueReceive(ipc_eye_queue(), &msg, pdMS_TO_TICKS(40));
        display_wait_idle();
        face_draw(fb[draw], LCD_WIDTH, msg.gaze.look_x, msg.gaze.look_y, msg.gaze.blink);
        const uint16_t *band = fb[draw] + (size_t)band_top * (size_t)LCD_WIDTH;
        display_blit(0, band_top, LCD_WIDTH, band_top + band_h, band);
        if (buffers == 2) {
            draw ^= 1;
        }
        frames++;
        uint32_t t = now_ms();
        if (t - fps_mark >= 2000) {
            ESP_LOGI(TAG, "display fps=%u", (unsigned)(frames * 1000 / (t - fps_mark)));
            frames = 0;
            fps_mark = t;
        }
    }
}
