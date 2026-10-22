#include "face.h"

#include <math.h>
#include <stddef.h>

static void hline(uint16_t *fb, int width, int x0, int x1, int y, uint16_t color)
{
    if (y < 0 || y >= LCD_HEIGHT) {
        return;
    }
    if (x0 > x1) {
        int tmp = x0;
        x0 = x1;
        x1 = tmp;
    }
    int dy = y - DIAL_CY;
    int remain = DIAL_R * DIAL_R - dy * dy;
    if (remain < 0) {
        return;
    }
    int dx_lim = (int)(sqrtf((float)remain) + 0.5f);
    int left = DIAL_CX - dx_lim;
    int right = DIAL_CX + dx_lim;
    if (x0 < left) {
        x0 = left;
    }
    if (x1 > right) {
        x1 = right;
    }
    if (x1 < 0 || x0 >= width) {
        return;
    }
    if (x0 < 0) {
        x0 = 0;
    }
    if (x1 >= width) {
        x1 = width - 1;
    }
    if (x0 > x1) {
        return;
    }
    uint16_t *row = fb + (size_t)y * (size_t)width + x0;
    for (int x = x0; x <= x1; ++x) {
        *row++ = color;
    }
}

static void fill_ellipse(uint16_t *fb, int width, int cx, int cy, int rx, int ry, uint16_t color)
{
    if (rx <= 0 || ry <= 0) {
        return;
    }
    for (int y = -ry; y <= ry; ++y) {
        float ny = (float)y / (float)ry;
        float span = 1.0f - ny * ny;
        if (span < 0) {
            continue;
        }
        int dx = (int)((float)rx * sqrtf(span) + 0.5f);
        hline(fb, width, cx - dx, cx + dx, cy + y, color);
    }
}

static void fill_rect(uint16_t *fb, int width, int x, int y, int w, int h, uint16_t color)
{
    for (int row = 0; row < h; ++row) {
        hline(fb, width, x, x + w - 1, y + row, color);
    }
}

static void draw_eye(uint16_t *fb, int width, int cx, int cy, int px, int py, float blink)
{
    fill_ellipse(fb, width, cx, cy, k_face.eye_rx, k_face.eye_ry, k_face.sclera);
    fill_ellipse(fb, width, cx + px, cy + py, k_face.iris_r, k_face.iris_r, k_face.iris);
    fill_ellipse(fb, width, cx + px, cy + py, k_face.pupil_r, k_face.pupil_r, k_face.pupil);
    fill_ellipse(fb, width, cx + px + k_face.catch_dx, cy + py + k_face.catch_dy, k_face.catch_r,
                 k_face.catch_r, k_face.highlight);
    if (blink > 0.02f) {
        int cover = (int)(blink * (float)(k_face.eye_ry * 2));
        int top = cy - k_face.eye_ry;
        fill_rect(fb, width, cx - k_face.eye_rx, top, k_face.eye_rx * 2, cover, k_face.bg);
    }
}

void face_clear(uint16_t *fb, int width, int height, uint16_t color)
{
    size_t n = (size_t)width * (size_t)height;
    for (size_t i = 0; i < n; ++i) {
        fb[i] = color;
    }
}

void face_draw(uint16_t *fb, int width, float look_x, float look_y, float blink)
{
    int band_h = k_face.band_bottom - k_face.band_top;
    uint16_t *band = fb + (size_t)k_face.band_top * (size_t)width;
    size_t pixels = (size_t)width * (size_t)band_h;
    for (size_t i = 0; i < pixels; ++i) {
        band[i] = k_face.bg;
    }

    int px = (int)(look_x * (float)FACE_MAX_PUPIL_PX);
    int py = (int)(look_y * (float)FACE_MAX_PUPIL_PX);
    if (px > FACE_MAX_PUPIL_PX) {
        px = FACE_MAX_PUPIL_PX;
    }
    if (px < -FACE_MAX_PUPIL_PX) {
        px = -FACE_MAX_PUPIL_PX;
    }
    if (py > FACE_MAX_PUPIL_PX) {
        py = FACE_MAX_PUPIL_PX;
    }
    if (py < -FACE_MAX_PUPIL_PX) {
        py = -FACE_MAX_PUPIL_PX;
    }

    draw_eye(fb, width, k_face.cx - k_face.eye_dx, k_face.eye_cy, px, py, blink);
    draw_eye(fb, width, k_face.cx + k_face.eye_dx, k_face.eye_cy, px, py, blink);
}
