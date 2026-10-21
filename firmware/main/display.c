#include "display.h"

#include "config.h"

#include "driver/spi_master.h"
#include "esp_lcd_co5300.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "display";

static esp_lcd_panel_handle_t s_panel;
static SemaphoreHandle_t s_flush_done;
static bool s_ready;

static bool on_color_done(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata,
                          void *user_ctx)
{
    (void)panel_io;
    (void)edata;
    SemaphoreHandle_t sem = (SemaphoreHandle_t)user_ctx;
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(sem, &woken);
    return woken == pdTRUE;
}

bool display_start(void)
{
    s_flush_done = xSemaphoreCreateBinary();
    if (!s_flush_done) {
        return false;
    }
    xSemaphoreGive(s_flush_done);

    const spi_bus_config_t buscfg = CO5300_PANEL_BUS_QSPI_CONFIG(
        LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3,
        LCD_WIDTH * LCD_HEIGHT * (int)sizeof(uint16_t));
    esp_err_t err = spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi bus: %s", esp_err_to_name(err));
        return false;
    }

    esp_lcd_panel_io_handle_t io = NULL;
    const esp_lcd_panel_io_spi_config_t io_config =
        CO5300_PANEL_IO_QSPI_CONFIG(LCD_CS, on_color_done, s_flush_done);
    err = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_config, &io);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "panel io: %s", esp_err_to_name(err));
        return false;
    }

    co5300_vendor_config_t vendor_config = {
        .flags.use_qspi_interface = 1,
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config = (void *)&vendor_config,
    };
    err = esp_lcd_new_panel_co5300(io, &panel_config, &s_panel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "co5300: %s", esp_err_to_name(err));
        return false;
    }
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    // HelloWorld passes column offset 6. draw_bitmap adds the gap itself.
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(s_panel, LCD_COL_OFFSET, 0));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_co5300_set_brightness(s_panel, LCD_BRIGHTNESS_PCT));
    s_ready = true;
    ESP_LOGI(TAG, "CO5300 %dx%d QSPI CS=%d SCLK=%d D0-D3=%d,%d,%d,%d RST=%d", LCD_WIDTH, LCD_HEIGHT,
             LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3, LCD_RST);
    return true;
}

bool display_ready(void)
{
    return s_ready;
}

void display_wait_idle(void)
{
    if (!s_flush_done) {
        return;
    }
    if (xSemaphoreTake(s_flush_done, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "flush wait timed out");
    }
}

bool display_blit(int x, int y, int x_end, int y_end, const void *pixels)
{
    if (!s_ready) {
        return false;
    }
    esp_err_t err = esp_lcd_panel_draw_bitmap(s_panel, x, y, x_end, y_end, pixels);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "blit: %s", esp_err_to_name(err));
        xSemaphoreGive(s_flush_done);
        return false;
    }
    return true;
}
