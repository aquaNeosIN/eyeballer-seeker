#pragma once

#include "sighting.h"

#include <stdbool.h>
#include <stdint.h>

// Screen-space look. 0 is center. ±1 is FACE_MAX_PUPIL_PX.
typedef struct {
    float look_x;
    float look_y;
    float blink;
    bool tracking;
    bool lingering;
    float angle_deg;
    float area;
} gaze_output_t;

typedef struct {
    uint32_t last_ms;
    int started;
    int has_last;
    int holding;
    uint32_t lost_ms;
    float hold_x;
    float hold_y;
    float look_x;
    float look_y;
    uint32_t blink_start_ms;
    uint32_t next_blink_ms;
    uint32_t next_glance_ms;
    uint32_t glance_until_ms;
    float glance_x;
    float glance_y;
    uint32_t rng;
} gaze_state_t;

void gaze_reset(gaze_state_t *state);
gaze_output_t gaze_update(gaze_state_t *state, const sighting_t *sight, uint32_t now_ms);
