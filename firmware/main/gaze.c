#include "gaze.h"

#include "config.h"

#include <math.h>

static float clampf(float value, float lo, float hi)
{
    if (value < lo) {
        return lo;
    }
    if (value > hi) {
        return hi;
    }
    return value;
}

static float hypot2(float x, float y)
{
    return sqrtf(x * x + y * y);
}

static int iabs16(int value)
{
    return value < 0 ? -value : value;
}

static uint32_t next_rng(gaze_state_t *state)
{
    state->rng = state->rng * 1664525u + 1013904223u;
    return state->rng;
}

static void smooth_toward(float *current, float target, uint32_t dt_ms, float tau_ms)
{
    if (dt_ms > 250) {
        dt_ms = 250;
    }
    float alpha = 1.0f - expf(-(float)dt_ms / tau_ms);
    *current += (target - *current) * alpha;
}

static bool acceptable(const radar_target_t *target, float *distance_mm)
{
    if (!target->present) {
        return false;
    }
    if (target->y_mm < MIN_FORWARD_MM) {
        return false;
    }
    float distance = hypot2((float)target->x_mm, (float)target->y_mm);
    *distance_mm = distance;
    if (distance < (float)OWNER_MIN_MM || distance > (float)MAX_RANGE_MM) {
        return false;
    }
    return true;
}

// Keep the person already locked when they are still near the last point.
// Otherwise take the nearest moving return, then the nearest return.
static int select_slot(gaze_state_t *state, const radar_target_t targets[LD2450_SLOTS])
{
    int near_last = -1;
    float near_last_gap = 1.0e9f;
    int best_moving = -1;
    float best_moving_d = 1.0e9f;
    int best_any = -1;
    float best_any_d = 1.0e9f;

    for (int i = 0; i < LD2450_SLOTS; ++i) {
        float distance = 0;
        if (!acceptable(&targets[i], &distance)) {
            continue;
        }
        if (distance < best_any_d) {
            best_any_d = distance;
            best_any = i;
        }
        if (iabs16(targets[i].speed_cm_s) >= MOVING_SPEED_CM_S && distance < best_moving_d) {
            best_moving_d = distance;
            best_moving = i;
        }
        if (state->has_last) {
            float gap = hypot2((float)targets[i].x_mm - state->last_x_mm,
                                (float)targets[i].y_mm - state->last_y_mm);
            if (gap <= (float)TRACK_HYSTERESIS_MM && gap < near_last_gap) {
                near_last_gap = gap;
                near_last = i;
            }
        }
    }

    int chosen = near_last;
    if (chosen < 0) {
        chosen = best_moving >= 0 ? best_moving : best_any;
    }
    if (chosen >= 0) {
        state->last_x_mm = (float)targets[chosen].x_mm;
        state->last_y_mm = (float)targets[chosen].y_mm;
        state->has_last = true;
    }
    return chosen;
}

static void look_from_target(const radar_target_t *target, float *look_x, float *look_y,
                             float *angle_deg, float *distance_mm)
{
    float x = (float)target->x_mm;
    float y = (float)target->y_mm;
    float distance = hypot2(x, y);
    float degrees = atan2f(x, y) * (180.0f / 3.14159265f);
    degrees = clampf(degrees, -ANGLE_LIMIT_DEG, ANGLE_LIMIT_DEG);
    float gain = (DIST_FAR_MM - distance) / (DIST_FAR_MM - DIST_NEAR_MM);
    gain = clampf(gain, DIST_GAIN_FLOOR, 1.0f);
    *look_x = (degrees / ANGLE_LIMIT_DEG) * gain;
    *look_y = (1.0f - gain) * 0.22f;
    *angle_deg = degrees;
    *distance_mm = distance;
}

static float blink_amount(gaze_state_t *state, uint32_t now_ms)
{
    if (state->blink_start_ms != 0) {
        uint32_t elapsed = now_ms - state->blink_start_ms;
        if (elapsed >= BLINK_MS) {
            state->blink_start_ms = 0;
        } else {
            float half = (float)BLINK_MS * 0.5f;
            float t = (float)elapsed;
            if (t < half) {
                return t / half;
            }
            return (2.0f * half - t) / half;
        }
    }
    if ((int32_t)(now_ms - state->next_blink_ms) >= 0) {
        state->blink_start_ms = now_ms;
        uint32_t jitter = next_rng(state) % 1800u;
        state->next_blink_ms = now_ms + BLINK_PERIOD_MS + jitter;
        return 0;
    }
    return 0;
}

static void idle_target(gaze_state_t *state, uint32_t now_ms, float *look_x, float *look_y)
{
    if ((int32_t)(now_ms - state->next_glance_ms) >= 0 &&
        (int32_t)(now_ms - state->glance_until_ms) >= 0) {
        float span = GLANCE_PX / (float)FACE_MAX_PUPIL_PX;
        int pick = (int)(next_rng(state) % 5u);
        state->glance_x = 0;
        state->glance_y = 0;
        if (pick == 0) {
            state->glance_x = -span;
        } else if (pick == 1) {
            state->glance_x = span;
        } else if (pick == 2) {
            state->glance_y = span * 0.4f;
        } else if (pick == 3) {
            state->glance_x = span * 0.5f;
            state->glance_y = -span * 0.25f;
        }
        state->glance_until_ms = now_ms + GLANCE_HOLD_MS;
        state->next_glance_ms = now_ms + GLANCE_PERIOD_MS + (next_rng(state) % 1500u);
    }
    if ((int32_t)(now_ms - state->glance_until_ms) < 0) {
        *look_x = state->glance_x;
        *look_y = state->glance_y;
    } else {
        *look_x = 0;
        *look_y = 0;
    }
}

void gaze_reset(gaze_state_t *state)
{
    state->last_ms = 0;
    state->started = false;
    state->has_last = false;
    state->last_x_mm = 0;
    state->last_y_mm = 0;
    state->holding = false;
    state->lost_ms = 0;
    state->hold_x = 0;
    state->hold_y = 0;
    state->look_x = 0;
    state->look_y = 0;
    state->blink_start_ms = 0;
    state->next_blink_ms = 900;
    state->next_glance_ms = 1200;
    state->glance_until_ms = 0;
    state->glance_x = 0;
    state->glance_y = 0;
    state->rng = 0xC0FFEEu;
}

gaze_output_t gaze_update(gaze_state_t *state, const radar_target_t targets[LD2450_SLOTS],
                          bool wrist_still, uint32_t now_ms)
{
    if (!state->started) {
        state->started = true;
        state->last_ms = now_ms;
        state->next_blink_ms = now_ms + 900;
        state->next_glance_ms = now_ms + 1200;
    }
    uint32_t dt = now_ms - state->last_ms;
    state->last_ms = now_ms;

    int slot = select_slot(state, targets);
    float desired_x = 0;
    float desired_y = 0;
    float angle = 0;
    float distance = 0;
    bool tracking = false;
    bool lingering = false;

    if (wrist_still && slot >= 0) {
        look_from_target(&targets[slot], &desired_x, &desired_y, &angle, &distance);
        state->holding = false;
        state->lost_ms = 0;
        tracking = true;
    } else if (wrist_still && state->has_last) {
        if (!state->holding) {
            state->holding = true;
            state->lost_ms = now_ms;
            state->hold_x = state->look_x;
            state->hold_y = state->look_y;
        }
        if (now_ms - state->lost_ms < LOST_LINGER_MS) {
            desired_x = state->hold_x;
            desired_y = state->hold_y;
            lingering = true;
            angle = state->hold_x * ANGLE_LIMIT_DEG;
        } else {
            state->has_last = false;
            state->holding = false;
            idle_target(state, now_ms, &desired_x, &desired_y);
        }
    } else {
        if (!wrist_still) {
            state->holding = false;
        }
        idle_target(state, now_ms, &desired_x, &desired_y);
        slot = -1;
    }

    float tau = (tracking || lingering) ? TRACK_TAU_MS : IDLE_TAU_MS;
    smooth_toward(&state->look_x, desired_x, dt, tau);
    smooth_toward(&state->look_y, desired_y, dt, tau);

    gaze_output_t out = {
        .look_x = state->look_x,
        .look_y = state->look_y,
        .blink = blink_amount(state, now_ms),
        .tracking = tracking,
        .lingering = lingering,
        .slot = tracking ? slot : -1,
        .angle_deg = angle,
        .distance_mm = distance,
    };
    return out;
}
