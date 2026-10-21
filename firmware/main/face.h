#pragma once

#include "config.h"

#include <stdint.h>

// Replace k_face, and face_draw() in face.c, to change the face.
// Coordinates are pixels on the 466x466 panel. +x is right, +y is down.
// look_x / look_y are multiplied by FACE_MAX_PUPIL_PX.

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
    .cx = 233,
    .eye_cy = 188,
    .eye_dx = 102,
    .eye_rx = 64,
    .eye_ry = 84,
    .iris_r = 32,
    .pupil_r = 15,
    .catch_r = 6,
    .catch_dx = -11,
    .catch_dy = -13,
    .band_top = 90,
    .band_bottom = 290,
};

void face_clear(uint16_t *fb, int width, int height, uint16_t color);
void face_draw(uint16_t *fb, int width, float look_x, float look_y, float blink);
