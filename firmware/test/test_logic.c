#include "gaze.h"
#include "track.h"

#include <stdio.h>
#include <string.h>

static int g_fails;

static void expect(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "FAIL %s\n", msg);
        g_fails++;
    }
}

static void test_motion(void)
{
    enum { W = 80, H = 60 };
    uint8_t prev_mem[W * H];
    uint8_t frame[W * H];
    track_state_t state;
    track_bind(&state, prev_mem, W * H);

    memset(frame, 0, sizeof(frame));
    sighting_t first = track_motion(&state, frame, W, H);
    expect(!first.present && !first.dropped_close, "first frame arms only");

    sighting_t still = track_motion(&state, frame, W, H);
    expect(!still.present && still.area == 0, "static frame is empty");

    memset(frame, 0, sizeof(frame));
    for (int y = 20; y < 40; ++y) {
        for (int x = 60; x < 80; ++x) {
            frame[y * W + x] = 255;
        }
    }
    sighting_t blob = track_motion(&state, frame, W, H);
    expect(blob.present, "right-side blob present");
    expect(!blob.dropped_close, "right-side blob is not too close");
    expect(blob.x > 0.4f, "blob is on the right");
    expect(blob.y > -0.45f && blob.y < 0.45f, "blob is near the vertical middle");

    memset(frame, 255, sizeof(frame));
    sighting_t close = track_motion(&state, frame, W, H);
    expect(!close.present && close.dropped_close, "full-frame motion is dropped");

    memset(frame, 0, sizeof(frame));
    track_motion(&state, frame, W, H);
    frame[10] = 255;
    frame[11] = 255;
    sighting_t speck = track_motion(&state, frame, W, H);
    expect(!speck.present && !speck.dropped_close, "speck is ignored");
}

static void test_luma(void)
{
    uint16_t src[4] = {0x0000, 0xFFFF, 0xF800, 0x001F};
    uint8_t dst[4];
    int w = 0;
    int h = 0;
    expect(track_rgb565_to_luma(src, 4, 1, 1, dst, 4, &w, &h), "luma converts");
    expect(w == 4 && h == 1, "luma size");
    expect(dst[0] < 5, "black luma");
    expect(dst[1] > 200, "white luma");
    expect(dst[2] > dst[0] && dst[3] > dst[0], "red and blue are above black");
}

static void test_gaze(void)
{
    gaze_state_t gaze;
    gaze_reset(&gaze);
    sighting_t seen = {
        .present = true,
        .x = 1.0f,
        .y = 0.0f,
        .area = 0.10f,
    };
    gaze_output_t look = gaze_update(&gaze, &seen, 1000);
    expect(look.tracking, "first sample tracks");
    look = gaze_update(&gaze, &seen, 1100);
    expect(look.tracking && look.look_x > 0.4f, "gaze moves toward the blob");

    sighting_t gone = {0};
    look = gaze_update(&gaze, &gone, 1200);
    expect(look.lingering && !look.tracking, "linger after the blob leaves");
    look = gaze_update(&gaze, &gone, 1200 + 2000);
    expect(!look.lingering && !look.tracking, "drift home after linger");
}

int main(void)
{
    test_luma();
    test_motion();
    test_gaze();
    if (g_fails) {
        fprintf(stderr, "%d failed\n", g_fails);
        return 1;
    }
    printf("logic ok\n");
    return 0;
}
