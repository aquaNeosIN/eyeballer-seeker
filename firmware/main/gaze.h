#pragma once

#include "ld2450.h"

#include <stdbool.h>
#include <stdint.h>

// Screen-space look. 0 is center. ±1 is FACE_MAX_PUPIL_PX.
typedef struct {
    float look_x;
    float look_y;
    float blink;
    bool tracking;
    bool lingering;
    int slot;
    float angle_deg;
    float distance_mm;
} gaze_output_t;

typedef struct {
    uint32_t last_ms;
    bool started;
    bool has_last;
    float last_x_mm;
    float last_y_mm;
    bool holding;
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
gaze_output_t gaze_update(gaze_state_t *state, const radar_target_t targets[LD2450_SLOTS],
                          bool wrist_still, uint32_t now_ms);
