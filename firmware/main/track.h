#pragma once

#include "sighting.h"

#include <stdint.h>

// Previous-frame memory is owned by the caller (see track_bind).
typedef struct {
    uint8_t *prev;
    int cap;
    int prev_w;
    int prev_h;
    int have_prev;
    int hold_cell;
} track_state_t;

void track_bind(track_state_t *state, uint8_t *prev, int cap);

// First call only stores the frame. Later calls return the strongest
// motion cluster, or an empty sighting when the change is tiny or
// covers too much of the frame.
sighting_t track_motion(track_state_t *state, const uint8_t *luma, int w, int h);

// Subsample native RGB565 (red in bits 15..11) into 8-bit luma.
// Returns 1 and writes *out_w / *out_h on success.
int track_rgb565_to_luma(const uint16_t *src, int src_w, int src_h, int step, uint8_t *dst,
                         int dst_cap, int *out_w, int *out_h);
