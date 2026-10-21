#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t board_i2c_init(void);
esp_err_t board_i2c_write(uint8_t addr_7bit, uint8_t reg, const uint8_t *data, size_t len);
esp_err_t board_i2c_read(uint8_t addr_7bit, uint8_t reg, uint8_t *data, size_t len);
