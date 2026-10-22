#include "board_io.h"
#include "camera_feed.h"
#include "config.h"
#include "display.h"
#include "ipc.h"
#include "tasks.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "app";

void app_main(void)
{
    ESP_LOGI(TAG, "eyeballer-seeker v2  DNESP32S3 + ST7789 240x280 + OV5640");
    ESP_LOGI(TAG, "motion tracker, not a face detector. simulate=%d", SIMULATE_CAMERA);

    if (!board_io_start()) {
        ESP_LOGE(TAG, "XL9555 init failed (camera reset and LCD reset)");
    }
    if (!display_start()) {
        ESP_LOGE(TAG, "display init failed");
    }
    if (!camera_feed_start()) {
        ESP_LOGW(TAG, "camera init failed, eyes will idle");
    }

    ipc_init();
    xTaskCreatePinnedToCore(cam_task, "cam", 8192, NULL, 4, NULL, 0);
    xTaskCreatePinnedToCore(eye_task, "eye", 4096, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(display_task, "disp", 6144, NULL, 5, NULL, 1);
}
