#pragma once

#include "config.h"

#include <stdint.h>

// The 1.69" module is a rounded rectangle, 240x280, not a round
// GC9A01. face_draw() paints the eyes inside a circular dial so the
// rectangle reads as a watch face. Pixels outside the dial stay black.
// The glass already clips the four corners; the mask is only software.
//
// Coordinates are pixels. +x is right, +y is down.
// look_x / look_y are multiplied by FACE_MAX_PUPIL_PX.

#define DIAL_CX 120
#define DIAL_CY 140
#define DIAL_R 116

typedef struct {
    uint16_t bg;
    uint16_t sclera;
    uint16_t iris;
    uint16_t pupil;
    uint16_t highlight;
    int16_t cx;
    int16_t eye_cy;
    int16_t eye_dx;
    int16_t eye_rx;
    int16_t eye_ry;
    int16_t iris_r;
    int16_t pupil_r;
    int16_t catch_r;
    int16_t catch_dx;
    int16_t catch_dy;
    int16_t band_top;
    int16_t band_bottom;
} face_art_t;

static const face_art_t k_face = {
    .bg = 0x0000,
    .sclera = 0xFFFF,
    .iris = 0x3A96,
    .pupil = 0x1082,
    .highlight = 0xFFFF,
    .cx = DIAL_CX,
    .eye_cy = 112,
    .eye_dx = 46,
    .eye_rx = 30,
    .eye_ry = 40,
    .iris_r = 14,
    .pupil_r = 6,
    .catch_r = 3,
    .catch_dx = -6,
    .catch_dy = -8,
    .band_top = 64,
    .band_bottom = 168,
};

void face_clear(uint16_t *fb, int width, int height, uint16_t color);
void face_draw(uint16_t *fb, int width, float look_x, float look_y, float blink);
