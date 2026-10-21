#pragma once

#include <stdbool.h>
#include <stdint.h>

// Hi-Link "LD2450 串口通信协议 V1.03", section 2.3.
// Frame: AA FF 03 00, three 8-byte targets, 55 CC.
#define LD2450_FRAME_LEN 30
#define LD2450_SLOTS 3

typedef struct {
    bool present;
    int16_t x_mm;
    int16_t y_mm;
    int16_t speed_cm_s;
    uint16_t resolution_mm;
} radar_target_t;

typedef struct {
    uint8_t buf[LD2450_FRAME_LEN];
    uint8_t len;
} ld2450_parser_t;

void ld2450_parser_reset(ld2450_parser_t *parser);
bool ld2450_decode_frame(const uint8_t *frame, radar_target_t out[LD2450_SLOTS]);
bool ld2450_parser_push(ld2450_parser_t *parser, uint8_t byte, radar_target_t out[LD2450_SLOTS]);

// Official worked example: x=-782 mm, y=1713 mm, speed=-16 cm/s, resolution=320 mm.
bool ld2450_self_test(void);

// Slot 0 is inside the owner radius. Slot 1 walks across the front. Slot 2 is empty.
void ld2450_build_sim_frame(uint32_t now_ms, uint8_t frame[LD2450_FRAME_LEN]);
