#include "config.h"
#include "display.h"
#include "i2c_bus.h"
#include "imu.h"
#include "ipc.h"
#include "ld2450.h"
#include "pmu.h"
#include "tasks.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "app";

void app_main(void)
{
    ESP_LOGI(TAG, "eyeballer-seeker v1  ESP-IDF");
    if (ld2450_self_test()) {
        ESP_LOGI(TAG, "LD2450 self-test passed");
    } else {
        ESP_LOGE(TAG, "LD2450 self-test failed");
    }

    ESP_ERROR_CHECK(board_i2c_init());
    pmu_probe();
    if (!display_start()) {
        ESP_LOGE(TAG, "display init failed");
    }
    if (!imu_start()) {
        ESP_LOGW(TAG, "IMU init failed");
    }

    ipc_init();
    xTaskCreatePinnedToCore(radar_task, "radar", 4096, NULL, 4, NULL, 0);
    xTaskCreatePinnedToCore(imu_task, "imu", 4096, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(eye_task, "eye", 6144, NULL, 4, NULL, 0);
    xTaskCreatePinnedToCore(display_task, "disp", 8192, NULL, 5, NULL, 1);
}
