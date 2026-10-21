#include "tasks.h"

#include "config.h"
#include "display.h"
#include "face.h"
#include "gaze.h"
#include "imu.h"
#include "ipc.h"
#include "ld2450.h"

#include "driver/uart.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

static const char *TAG = "tasks";

static uint32_t now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static void publish_frame(const radar_target_t slots[LD2450_SLOTS])
{
    radar_msg_t msg = {.time_ms = now_ms()};
    memcpy(msg.slot, slots, sizeof(msg.slot));
    xQueueOverwrite(ipc_radar_queue(), &msg);
}

void radar_task(void *arg)
{
    (void)arg;
    ld2450_parser_t parser;
    ld2450_parser_reset(&parser);
    radar_target_t slots[LD2450_SLOTS];

    if (SIMULATE_RADAR) {
        ESP_LOGI(TAG, "radar simulation on");
        while (true) {
            uint8_t frame[LD2450_FRAME_LEN];
            ld2450_build_sim_frame(now_ms(), frame);
            bool got = false;
            for (int i = 0; i < LD2450_FRAME_LEN; ++i) {
                if (ld2450_parser_push(&parser, frame[i], slots)) {
                    got = true;
                }
            }
            if (got) {
                publish_frame(slots);
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    uart_config_t cfg = {
        .baud_rate = RADAR_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(RADAR_UART, 2048, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(RADAR_UART, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(RADAR_UART, RADAR_TX_PIN, RADAR_RX_PIN, UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE));
    ESP_LOGI(TAG, "UART1 TX=%d RX=%d baud=%d", RADAR_TX_PIN, RADAR_RX_PIN, RADAR_BAUD);
    uint8_t buf[128];
    while (true) {
        int n = uart_read_bytes(RADAR_UART, buf, sizeof(buf), pdMS_TO_TICKS(50));
        for (int i = 0; i < n; ++i) {
            if (ld2450_parser_push(&parser, buf[i], slots)) {
                publish_frame(slots);
            }
        }
    }
}

void imu_task(void *arg)
{
    (void)arg;
    uint32_t changed_ms = 0;
    bool still = true;
    ipc_publish_motion(true, NULL);
    if (!imu_ready()) {
        ESP_LOGW(TAG, "IMU missing, eyes keep tracking");
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
    while (true) {
        imu_sample_t sample;
        if (imu_read(&sample)) {
            bool moving_now = imu_wrist_moving(&sample);
            uint32_t t = now_ms();
            if (moving_now && still) {
                if (changed_ms == 0) {
                    changed_ms = t;
                }
                if (t - changed_ms >= MOTION_ENTER_MS) {
                    still = false;
                    changed_ms = 0;
                }
            } else if (!moving_now && !still) {
                if (changed_ms == 0) {
                    changed_ms = t;
                }
                if (t - changed_ms >= STILL_ENTER_MS) {
                    still = true;
                    changed_ms = 0;
                }
            } else {
                changed_ms = 0;
            }
            ipc_publish_motion(still, &sample);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void eye_task(void *arg)
{
    (void)arg;
    gaze_state_t gaze;
    gaze_reset(&gaze);
    radar_msg_t radar = {0};
    bool have_radar = false;
    uint32_t last_log = 0;
    TickType_t wake = xTaskGetTickCount();

    while (true) {
        radar_msg_t incoming;
        if (xQueueReceive(ipc_radar_queue(), &incoming, 0) == pdTRUE) {
            radar = incoming;
            have_radar = true;
        }
        uint32_t t = now_ms();
        radar_target_t blank[LD2450_SLOTS] = {0};
        const radar_target_t *targets = blank;
        if (have_radar && t - radar.time_ms < RADAR_STALE_MS) {
            targets = radar.slot;
        }
        bool still = ipc_wrist_still(NULL);
        gaze_output_t look = gaze_update(&gaze, targets, still, t);

        eye_msg_t msg = {
            .gaze = look,
            .wrist_still = still,
        };
        memcpy(msg.slot, targets, sizeof(msg.slot));
        xQueueOverwrite(ipc_eye_queue(), &msg);

        if (t - last_log >= DEBUG_PERIOD_MS) {
            last_log = t;
            const char *mode = look.tracking ? "track" : (look.lingering ? "linger" : "idle");
            ESP_LOGI(TAG, "%s slot=%d ang=%d dist=%d look=%d,%d still=%d", mode, look.slot,
                     (int)look.angle_deg, (int)look.distance_mm, (int)(look.look_x * 100),
                     (int)(look.look_y * 100), still ? 1 : 0);
            for (int i = 0; i < LD2450_SLOTS; ++i) {
                if (!targets[i].present) {
                    ESP_LOGI(TAG, "  t%d empty", i);
                    continue;
                }
                ESP_LOGI(TAG, "  t%d x=%d y=%d v=%d", i, targets[i].x_mm, targets[i].y_mm,
                         targets[i].speed_cm_s);
            }
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
    uint16_t *fb[2];
    for (int i = 0; i < 2; ++i) {
        fb[i] = heap_caps_malloc(pixels * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!fb[i]) {
            ESP_LOGE(TAG, "framebuffer %d alloc failed", i);
            while (true) {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
        face_clear(fb[i], LCD_WIDTH, LCD_HEIGHT, k_face.bg);
    }

    display_wait_idle();
    display_blit(0, 0, LCD_WIDTH, LCD_HEIGHT, fb[0]);

    int draw = 1;
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
        draw ^= 1;
        frames++;
        uint32_t t = now_ms();
        if (t - fps_mark >= 2000) {
            ESP_LOGI(TAG, "display fps=%u", (unsigned)(frames * 1000 / (t - fps_mark)));
            frames = 0;
            fps_mark = t;
        }
    }
}
