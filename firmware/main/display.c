#include "display.h"

#include "board_io.h"
#include "config.h"

#include "driver/spi_master.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"
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

    const spi_bus_config_t bus = {
        .mosi_io_num = LCD_MOSI_PIN,
        .miso_io_num = -1,
        .sclk_io_num = LCD_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        // Chunk size for the SPI driver. The full frame is larger than
        // internal DMA memory, so the panel IO splits the blit.
        .max_transfer_sz = 16 * 1024,
    };
    esp_err_t err = spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi bus: %s", esp_err_to_name(err));
        return false;
    }

    esp_lcd_panel_io_handle_t io = NULL;
    const esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = LCD_CS_PIN,
        .dc_gpio_num = LCD_DC_PIN,
        .spi_mode = 0,
        .pclk_hz = LCD_SPI_HZ,
        .trans_queue_depth = 4,
        .on_color_trans_done = on_color_done,
        .user_ctx = s_flush_done,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .flags.psram_dma_direct = 1,
    };
    err = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "panel io: %s", esp_err_to_name(err));
        return false;
    }

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_BGR ? LCD_RGB_ELEMENT_ORDER_BGR : LCD_RGB_ELEMENT_ORDER_RGB,
        // Framebuffer pixels are native little-endian RGB565. The ST7789
        // RAMCTL bit is set to match, so the driver does not swap bytes.
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = 16,
    };
    err = esp_lcd_new_panel_st7789(io, &panel_config, &s_panel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "st7789: %s", esp_err_to_name(err));
        return false;
    }
    err = esp_lcd_panel_init(s_panel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "panel init: %s", esp_err_to_name(err));
        return false;
    }
    esp_lcd_panel_invert_color(s_panel, LCD_INVERT_COLOR ? true : false);
    esp_lcd_panel_mirror(s_panel, LCD_MIRROR_X ? true : false, LCD_MIRROR_Y ? true : false);
    esp_lcd_panel_swap_xy(s_panel, false);
    esp_lcd_panel_set_gap(s_panel, LCD_COL_OFFSET, LCD_ROW_OFFSET);
    esp_lcd_panel_disp_on_off(s_panel, true);
    board_io_backlight(true);
    s_ready = true;
    ESP_LOGI(TAG, "ST7789V2 %dx%d SPI MOSI=%d SCK=%d CS=%d DC=%d gap=%d,%d invert=%d", LCD_WIDTH,
             LCD_HEIGHT, LCD_MOSI_PIN, LCD_SCK_PIN, LCD_CS_PIN, LCD_DC_PIN, LCD_COL_OFFSET,
             LCD_ROW_OFFSET, LCD_INVERT_COLOR);
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
    if (xSemaphoreTake(s_flush_done, pdMS_TO_TICKS(200)) != pdTRUE) {
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
