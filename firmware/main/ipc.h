#pragma once

#include "gaze.h"
#include "sighting.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <stdint.h>

typedef struct {
    sighting_t sight;
    uint32_t time_ms;
} sight_msg_t;

typedef struct {
    gaze_output_t gaze;
    sighting_t sight;
} eye_msg_t;

void ipc_init(void);
QueueHandle_t ipc_sight_queue(void);
QueueHandle_t ipc_eye_queue(void);
