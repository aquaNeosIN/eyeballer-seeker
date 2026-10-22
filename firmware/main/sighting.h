#pragma once

#include <stdbool.h>

// One observation from the camera task.
// x: -1 left, +1 right. y: -1 up, +1 down. area: 0..1 of the sample.
typedef struct {
    bool present;
    bool dropped_close;
    float x;
    float y;
    float area;
} sighting_t;
