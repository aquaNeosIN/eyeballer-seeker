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
    state->started = 0;
    state->has_last = 0;
    state->holding = 0;
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

gaze_output_t gaze_update(gaze_state_t *state, const sighting_t *sight, uint32_t now_ms)
{
    if (!state->started) {
        state->started = 1;
        state->last_ms = now_ms;
        state->next_blink_ms = now_ms + 900;
        state->next_glance_ms = now_ms + 1200;
    }
    uint32_t dt = now_ms - state->last_ms;
    state->last_ms = now_ms;

    int have = sight && sight->present;
    float desired_x = 0;
    float desired_y = 0;
    float angle = 0;
    float area = sight ? sight->area : 0;
    int tracking = 0;
    int lingering = 0;

    if (have) {
        float gain = clampf(sight->area / AREA_GAIN_REF, AREA_GAIN_FLOOR, 1.0f);
        desired_x = clampf(sight->x, -1.0f, 1.0f) * gain;
        desired_y = clampf(sight->y, -1.0f, 1.0f) * LOOK_Y_SCALE * gain;
        angle = desired_x * ANGLE_LIMIT_DEG;
        state->has_last = 1;
        state->holding = 0;
        state->lost_ms = 0;
        tracking = 1;
    } else if (state->has_last) {
        if (!state->holding) {
            state->holding = 1;
            state->lost_ms = now_ms;
            state->hold_x = state->look_x;
            state->hold_y = state->look_y;
        }
        if (now_ms - state->lost_ms < LOST_LINGER_MS) {
            desired_x = state->hold_x;
            desired_y = state->hold_y;
            angle = state->hold_x * ANGLE_LIMIT_DEG;
            lingering = 1;
        } else {
            state->has_last = 0;
            state->holding = 0;
            idle_target(state, now_ms, &desired_x, &desired_y);
        }
    } else {
        idle_target(state, now_ms, &desired_x, &desired_y);
    }

    float tau = (tracking || lingering) ? TRACK_TAU_MS : IDLE_TAU_MS;
    smooth_toward(&state->look_x, desired_x, dt, tau);
    smooth_toward(&state->look_y, desired_y, dt, tau);

    gaze_output_t result = {
        .look_x = state->look_x,
        .look_y = state->look_y,
        .blink = blink_amount(state, now_ms),
        .tracking = tracking ? true : false,
        .lingering = lingering ? true : false,
        .angle_deg = angle,
        .area = area,
    };
    return result;
}
