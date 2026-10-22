#include "track.h"

#include "config.h"

#include <string.h>

void track_bind(track_state_t *state, uint8_t *prev, int cap)
{
    state->prev = prev;
    state->cap = cap;
    state->prev_w = 0;
    state->prev_h = 0;
    state->have_prev = 0;
    state->hold_cell = -1;
}

static uint8_t luma_of(uint16_t pixel)
{
    int r = (pixel >> 11) & 31;
    int g = (pixel >> 5) & 63;
    int b = pixel & 31;
    int y = (r * 8 * 3 + g * 4 * 6 + b * 8) / 10;
    if (y > 255) {
        y = 255;
    }
    return (uint8_t)y;
}

int track_rgb565_to_luma(const uint16_t *src, int src_w, int src_h, int step, uint8_t *dst,
                         int dst_cap, int *out_w, int *out_h)
{
    if (!src || !dst || !out_w || !out_h || step < 1 || src_w < 1 || src_h < 1) {
        return 0;
    }
    int dw = src_w / step;
    int dh = src_h / step;
    if (dw < 1 || dh < 1 || dw * dh > dst_cap) {
        return 0;
    }
    for (int y = 0; y < dh; ++y) {
        const uint16_t *row = src + (size_t)(y * step) * (size_t)src_w;
        uint8_t *out = dst + (size_t)y * (size_t)dw;
        for (int x = 0; x < dw; ++x) {
            out[x] = luma_of(row[x * step]);
        }
    }
    *out_w = dw;
    *out_h = dh;
    return 1;
}

static sighting_t empty_sight(void)
{
    sighting_t out = {0};
    return out;
}

sighting_t track_motion(track_state_t *state, const uint8_t *luma, int w, int h)
{
    sighting_t out = empty_sight();
    if (!state || !state->prev || !luma || w < TRACK_CELLS_X || h < TRACK_CELLS_Y) {
        return out;
    }
    int n = w * h;
    if (n > state->cap || n > TRACK_MAX_PIXELS) {
        return out;
    }
    if (!state->have_prev || state->prev_w != w || state->prev_h != h) {
        memcpy(state->prev, luma, (size_t)n);
        state->prev_w = w;
        state->prev_h = h;
        state->have_prev = 1;
        state->hold_cell = -1;
        return out;
    }

    const int cell_w = w / TRACK_CELLS_X;
    const int cell_h = h / TRACK_CELLS_Y;
    const int scan_w = cell_w * TRACK_CELLS_X;
    const int scan_h = cell_h * TRACK_CELLS_Y;
    int cells[TRACK_CELLS_Y * TRACK_CELLS_X];
    memset(cells, 0, sizeof(cells));

    int moved = 0;
    for (int y = 0; y < scan_h; ++y) {
        const uint8_t *row = luma + (size_t)y * (size_t)w;
        const uint8_t *old = state->prev + (size_t)y * (size_t)w;
        int cy = y / cell_h;
        for (int x = 0; x < scan_w; ++x) {
            int delta = (int)row[x] - (int)old[x];
            if (delta < 0) {
                delta = -delta;
            }
            if (delta < TRACK_MOTION_DELTA) {
                continue;
            }
            cells[cy * TRACK_CELLS_X + (x / cell_w)]++;
            moved++;
        }
    }
    memcpy(state->prev, luma, (size_t)n);

    float area = (float)moved / (float)(w * h);
    out.area = area;
    if (area > TRACK_MAX_AREA) {
        out.dropped_close = true;
        state->hold_cell = -1;
        return out;
    }
    if (area < TRACK_MIN_AREA) {
        state->hold_cell = -1;
        return out;
    }

    int scores[TRACK_CELLS_Y * TRACK_CELLS_X];
    int best = 0;
    int best_i = -1;
    for (int i = 0; i < TRACK_CELLS_X * TRACK_CELLS_Y; ++i) {
        scores[i] = cells[i];
        if (i == state->hold_cell && cells[i] > 0) {
            scores[i] += cells[i] / 2;
        }
        if (scores[i] > best) {
            best = scores[i];
            best_i = i;
        }
    }
    if (best_i < 0 || cells[best_i] == 0) {
        state->hold_cell = -1;
        return out;
    }

    int bx = best_i % TRACK_CELLS_X;
    int by = best_i / TRACK_CELLS_X;
    float sx = 0;
    float sy = 0;
    float sw = 0;
    for (int cy = 0; cy < TRACK_CELLS_Y; ++cy) {
        for (int cx = 0; cx < TRACK_CELLS_X; ++cx) {
            int dx = cx - bx;
            int dy = cy - by;
            if (dx < 0) {
                dx = -dx;
            }
            if (dy < 0) {
                dy = -dy;
            }
            if (dx > 1 || dy > 1) {
                continue;
            }
            int count = cells[cy * TRACK_CELLS_X + cx];
            if (count <= 0 || count * 4 < cells[best_i]) {
                continue;
            }
            float px = ((float)cx + 0.5f) * (float)cell_w;
            float py = ((float)cy + 0.5f) * (float)cell_h;
            sx += px * (float)count;
            sy += py * (float)count;
            sw += (float)count;
        }
    }
    if (sw <= 0) {
        state->hold_cell = -1;
        return out;
    }

    float u = (sx / sw) / (float)w;
    float v = (sy / sw) / (float)h;
    out.present = true;
    out.x = u * 2.0f - 1.0f;
    out.y = v * 2.0f - 1.0f;
    state->hold_cell = best_i;
    return out;
}
