#include "ld2450.h"

#include <math.h>
#include <string.h>

// Sign bit is not two's complement. Protocol PDF V1.03:
// bit 15 set -> positive magnitude (raw - 0x8000)
// bit 15 clear -> negative magnitude
static int16_t decode_signed(uint16_t raw)
{
    if (raw & 0x8000) {
        return (int16_t)(raw - 0x8000);
    }
    return (int16_t)(-(int32_t)raw);
}

static uint16_t encode_signed(int16_t value)
{
    if (value >= 0) {
        return (uint16_t)value | 0x8000;
    }
    return (uint16_t)(-(int32_t)value);
}

static uint16_t read_u16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static void write_u16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value & 0xFF);
    p[1] = (uint8_t)(value >> 8);
}

void ld2450_parser_reset(ld2450_parser_t *parser)
{
    parser->len = 0;
}

bool ld2450_decode_frame(const uint8_t *frame, radar_target_t out[LD2450_SLOTS])
{
    if (frame[0] != 0xAA || frame[1] != 0xFF || frame[2] != 0x03 || frame[3] != 0x00) {
        return false;
    }
    if (frame[28] != 0x55 || frame[29] != 0xCC) {
        return false;
    }
    for (int i = 0; i < LD2450_SLOTS; ++i) {
        const uint8_t *block = frame + 4 + i * 8;
        bool blank = true;
        for (int b = 0; b < 8; ++b) {
            if (block[b] != 0) {
                blank = false;
                break;
            }
        }
        radar_target_t *slot = &out[i];
        if (blank) {
            slot->present = false;
            slot->x_mm = 0;
            slot->y_mm = 0;
            slot->speed_cm_s = 0;
            slot->resolution_mm = 0;
            continue;
        }
        slot->x_mm = decode_signed(read_u16(block));
        slot->y_mm = decode_signed(read_u16(block + 2));
        slot->speed_cm_s = decode_signed(read_u16(block + 4));
        slot->resolution_mm = read_u16(block + 6);
        slot->present = !(slot->x_mm == 0 && slot->y_mm == 0);
    }
    return true;
}

bool ld2450_parser_push(ld2450_parser_t *parser, uint8_t byte, radar_target_t out[LD2450_SLOTS])
{
    if (parser->len == 0) {
        if (byte != 0xAA) {
            return false;
        }
        parser->buf[0] = byte;
        parser->len = 1;
        return false;
    }

    parser->buf[parser->len++] = byte;

    if (parser->len == 2 && parser->buf[1] != 0xFF) {
        parser->len = (byte == 0xAA) ? 1 : 0;
        if (parser->len == 1) {
            parser->buf[0] = 0xAA;
        }
        return false;
    }
    if (parser->len == 3 && parser->buf[2] != 0x03) {
        parser->len = 0;
        return false;
    }
    if (parser->len == 4 && parser->buf[3] != 0x00) {
        parser->len = 0;
        return false;
    }
    if (parser->len < LD2450_FRAME_LEN) {
        return false;
    }

    parser->len = 0;
    return ld2450_decode_frame(parser->buf, out);
}

bool ld2450_self_test(void)
{
    // Bytes from the worked example in protocol PDF V1.03 section 2.3.
    static const uint8_t frame[LD2450_FRAME_LEN] = {
        0xAA, 0xFF, 0x03, 0x00, 0x0E, 0x03, 0xB1, 0x86, 0x10, 0x00, 0x40, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x55, 0xCC};
    radar_target_t out[LD2450_SLOTS];
    if (!ld2450_decode_frame(frame, out)) {
        return false;
    }
    if (!out[0].present || out[1].present || out[2].present) {
        return false;
    }
    return out[0].x_mm == -782 && out[0].y_mm == 1713 && out[0].speed_cm_s == -16 &&
           out[0].resolution_mm == 320;
}

static void write_slot(uint8_t *block, int16_t x_mm, int16_t y_mm, int16_t speed_cm_s,
                       uint16_t resolution_mm)
{
    write_u16(block, encode_signed(x_mm));
    write_u16(block + 2, encode_signed(y_mm));
    write_u16(block + 4, encode_signed(speed_cm_s));
    write_u16(block + 6, resolution_mm);
}

void ld2450_build_sim_frame(uint32_t now_ms, uint8_t frame[LD2450_FRAME_LEN])
{
    memset(frame, 0, LD2450_FRAME_LEN);
    frame[0] = 0xAA;
    frame[1] = 0xFF;
    frame[2] = 0x03;
    frame[3] = 0x00;
    frame[28] = 0x55;
    frame[29] = 0xCC;

    write_slot(frame + 4, 0, 300, 0, 320);

    const float period_ms = 7000.0f;
    const float phase = (float)now_ms / period_ms * 6.2831853f;
    const float x_mm = 1400.0f * sinf(phase);
    const float speed = 1400.0f * cosf(phase) * (6.2831853f / 7.0f) / 10.0f;
    int16_t speed_cm_s = (int16_t)speed;
    if (speed_cm_s == 0) {
        speed_cm_s = 1;
    }
    write_slot(frame + 12, (int16_t)x_mm, 1800, speed_cm_s, 320);
}
