#include "gaze.h"
#include "ld2450.h"

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

int main(void)
{
    expect(ld2450_self_test(), "protocol example");

    uint8_t frame[LD2450_FRAME_LEN];
    ld2450_build_sim_frame(0, frame);
    ld2450_parser_t parser;
    ld2450_parser_reset(&parser);
    radar_target_t slots[LD2450_SLOTS];
    int got = 0;
    for (int i = 0; i < LD2450_FRAME_LEN; ++i) {
        if (ld2450_parser_push(&parser, frame[i], slots)) {
            got++;
        }
    }
    expect(got == 1, "sim frame parses");
    expect(slots[0].present && slots[0].y_mm == 300, "owner slot present");
    expect(slots[1].present && slots[1].y_mm == 1800, "walker present");
    expect(!slots[2].present, "slot 2 empty");

    gaze_state_t gaze;
    gaze_reset(&gaze);
    gaze_output_t look = gaze_update(&gaze, slots, true, 1000);
    expect(look.tracking && look.slot == 1, "owner ignored, walker chosen");
    expect(look.distance_mm > 500.0f, "chosen distance outside owner radius");

    radar_target_t gone[LD2450_SLOTS] = {0};
    look = gaze_update(&gaze, gone, true, 1100);
    expect(look.lingering, "linger after loss");
    look = gaze_update(&gaze, gone, true, 1100 + 2000);
    expect(!look.lingering && !look.tracking, "drift home after linger");

    if (g_fails) {
        fprintf(stderr, "%d failed\n", g_fails);
        return 1;
    }
    printf("logic ok\n");
    return 0;
}
