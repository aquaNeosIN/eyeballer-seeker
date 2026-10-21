#pragma once

#include "gaze.h"
#include "imu.h"
#include "ld2450.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct {
    radar_target_t slot[LD2450_SLOTS];
    uint32_t time_ms;
} radar_msg_t;

typedef struct {
    gaze_output_t gaze;
    radar_target_t slot[LD2450_SLOTS];
    bool wrist_still;
} eye_msg_t;

void ipc_init(void);
QueueHandle_t ipc_radar_queue(void);
QueueHandle_t ipc_eye_queue(void);

void ipc_publish_motion(bool wrist_still, const imu_sample_t *sample);
bool ipc_wrist_still(imu_sample_t *sample_out);
