#include "board_io.h"

#include "config.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "board";

static i2c_master_dev_handle_t s_xl;
static uint16_t s_out = 0xFFFF;
static bool s_ready;

static esp_err_t xl_write(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return i2c_master_transmit(s_xl, buf, sizeof(buf), 100);
}

static esp_err_t xl_flush(void)
{
    esp_err_t err = xl_write(0x02, (uint8_t)(s_out & 0xFF));
    if (err != ESP_OK) {
        return err;
    }
    return xl_write(0x03, (uint8_t)((s_out >> 8) & 0xFF));
}

static void xl_level(int bit, int level)
{
    uint16_t mask = (uint16_t)1u << bit;
    if (level) {
        s_out |= mask;
    } else {
        s_out &= (uint16_t)~mask;
    }
}

static void direct_lcd_pins(void)
{
#if !LCD_USE_XL9555
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << LCD_RST_GPIO) | (1ULL << LCD_BL_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    gpio_set_level(LCD_BL_GPIO, 0);
    gpio_set_level(LCD_RST_GPIO, 1);
#endif
}

static void lcd_reset_pulse(void)
{
#if LCD_USE_XL9555
    xl_level(XL_LCD_RST, 1);
    xl_flush();
    vTaskDelay(pdMS_TO_TICKS(10));
    xl_level(XL_LCD_RST, 0);
    xl_flush();
    vTaskDelay(pdMS_TO_TICKS(20));
    xl_level(XL_LCD_RST, 1);
    xl_flush();
    vTaskDelay(pdMS_TO_TICKS(120));
#else
    gpio_set_level(LCD_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(LCD_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(LCD_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
#endif
}

bool board_io_start(void)
{
    direct_lcd_pins();

    i2c_master_bus_config_t bus = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t i2c = NULL;
    esp_err_t err = i2c_new_master_bus(&bus, &i2c);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c bus: %s", esp_err_to_name(err));
        return false;
    }

    i2c_device_config_t dev = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = XL9555_ADDR,
        .scl_speed_hz = I2C_HZ,
    };
    err = i2c_master_bus_add_device(i2c, &dev, &s_xl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "xl9555 add: %s", esp_err_to_name(err));
        return false;
    }

    // Port 0: IO0_0 and IO0_1 inputs, the rest outputs.
    // Port 1: IO1_0..IO1_3 outputs (LCD), IO1_4..IO1_7 inputs (keys).
    err = xl_write(0x06, 0x03);
    if (err == ESP_OK) {
        err = xl_write(0x07, 0xF0);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "xl9555 config: %s", esp_err_to_name(err));
        return false;
    }

    s_out = 0xFFFF;
    xl_level(XL_BEEP, XL_BEEP_OFF);
    xl_level(XL_OV_PWDN, 0);
    xl_level(XL_OV_RESET, 0);
    xl_level(XL_LCD_BL, 0);
    xl_level(XL_LCD_RST, 1);
    err = xl_flush();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "xl9555 write: %s", esp_err_to_name(err));
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
    xl_level(XL_OV_RESET, 1);
    err = xl_flush();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "camera reset release: %s", esp_err_to_name(err));
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(50));

    lcd_reset_pulse();
    s_ready = true;
    ESP_LOGI(TAG, "XL9555 0x%02x  cam pwdn=IO0_%d reset=IO0_%d  lcd rst=%s bl=%s", XL9555_ADDR,
             XL_OV_PWDN, XL_OV_RESET, LCD_USE_XL9555 ? "IO1_2" : "GPIO",
             LCD_USE_XL9555 ? "IO1_3" : "GPIO");
    return true;
}

bool board_io_ready(void)
{
    return s_ready;
}

void board_io_backlight(bool on)
{
    if (!s_ready && LCD_USE_XL9555) {
        return;
    }
#if LCD_USE_XL9555
    xl_level(XL_LCD_BL, on ? 1 : 0);
    if (xl_flush() != ESP_OK) {
        ESP_LOGW(TAG, "backlight write failed");
    }
#else
    gpio_set_level(LCD_BL_GPIO, on ? 1 : 0);
#endif
}
