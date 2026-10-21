#include "i2c_bus.h"

#include "config.h"

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "i2c";

static i2c_master_bus_handle_t s_bus;
static i2c_master_dev_handle_t s_qmi;
static i2c_master_dev_handle_t s_axp;

static i2c_master_dev_handle_t device_for(uint8_t addr)
{
    if (addr == QMI8658_ADDR) {
        return s_qmi;
    }
    if (addr == AXP2101_ADDR) {
        return s_axp;
    }
    return NULL;
}

esp_err_t board_i2c_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &s_bus), TAG, "i2c bus");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = QMI8658_ADDR,
        .scl_speed_hz = I2C_HZ,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(s_bus, &dev_cfg, &s_qmi), TAG, "qmi device");
    dev_cfg.device_address = AXP2101_ADDR;
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(s_bus, &dev_cfg, &s_axp), TAG, "axp device");
    ESP_LOGI(TAG, "I2C SDA=%d SCL=%d %d Hz", I2C_SDA, I2C_SCL, I2C_HZ);
    return ESP_OK;
}

esp_err_t board_i2c_write(uint8_t addr_7bit, uint8_t reg, const uint8_t *data, size_t len)
{
    i2c_master_dev_handle_t dev = device_for(addr_7bit);
    if (!dev) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t buf[8];
    if (len + 1 > sizeof(buf)) {
        return ESP_ERR_INVALID_SIZE;
    }
    buf[0] = reg;
    for (size_t i = 0; i < len; ++i) {
        buf[i + 1] = data[i];
    }
    return i2c_master_transmit(dev, buf, len + 1, 50);
}

esp_err_t board_i2c_read(uint8_t addr_7bit, uint8_t reg, uint8_t *data, size_t len)
{
    i2c_master_dev_handle_t dev = device_for(addr_7bit);
    if (!dev) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_transmit_receive(dev, &reg, 1, data, len, 50);
}
