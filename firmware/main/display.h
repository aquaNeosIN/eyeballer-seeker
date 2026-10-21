#pragma once

#include <stdbool.h>

bool display_start(void);
bool display_ready(void);

// Copy a finished RGB565 frame. x_end / y_end are exclusive.
bool display_blit(int x, int y, int x_end, int y_end, const void *pixels);

// Blocks until the previous blit has finished, with a timeout.
void display_wait_idle(void);
