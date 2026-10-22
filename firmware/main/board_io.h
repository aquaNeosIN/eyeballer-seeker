#pragma once

#include <stdbool.h>

// I2C + XL9555. Releases the camera from reset and pulses LCD reset.
// Backlight stays off until board_io_backlight(true).
bool board_io_start(void);
bool board_io_ready(void);
void board_io_backlight(bool on);
