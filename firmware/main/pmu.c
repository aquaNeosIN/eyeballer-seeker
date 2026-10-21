#include "pmu.h"

#include "config.h"
#include "i2c_bus.h"

#include "esp_log.h"

static const char *TAG = "pmu";

#define AXP2101_REG_CHIP_ID 0x03
#define AXP2101_CHIP_ID 0x4A

bool pmu_probe(void)
{
    uint8_t id = 0;
    esp_err_t err = board_i2c_read(AXP2101_ADDR, AXP2101_REG_CHIP_ID, &id, 1);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "AXP2101 not answering at 0x%02X (%s)", AXP2101_ADDR, esp_err_to_name(err));
        return false;
    }
    if (id != AXP2101_CHIP_ID) {
        ESP_LOGW(TAG, "AXP2101 chip id 0x%02X, expected 0x%02X", id, AXP2101_CHIP_ID);
        return false;
    }
    ESP_LOGI(TAG, "AXP2101 online, chip id 0x%02X", id);
    return true;
}
