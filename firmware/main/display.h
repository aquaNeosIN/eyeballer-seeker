#pragma once

#include <stdbool.h>

bool display_start(void);
bool display_ready(void);

// Copy a finished RGB565 rectangle. x_end / y_end are exclusive.
bool display_blit(int x, int y, int x_end, int y_end, const void *pixels);
void display_wait_idle(void);
