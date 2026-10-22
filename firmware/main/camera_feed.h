#pragma once

#include "sighting.h"

#include <stdbool.h>

// Opens the sensor unless SIMULATE_CAMERA is set.
// A false return means later polls publish an empty sighting.
bool camera_feed_start(void);
void camera_feed_poll(sighting_t *out);
