#include "ipc.h"

#include "freertos/semphr.h"

static QueueHandle_t s_radar;
static QueueHandle_t s_eye;
static SemaphoreHandle_t s_imu_mu;
static bool s_wrist_still = true;
static imu_sample_t s_sample;
static bool s_have_sample;

void ipc_init(void)
{
    s_radar = xQueueCreate(1, sizeof(radar_msg_t));
    s_eye = xQueueCreate(1, sizeof(eye_msg_t));
    s_imu_mu = xSemaphoreCreateMutex();
}

QueueHandle_t ipc_radar_queue(void)
{
    return s_radar;
}

QueueHandle_t ipc_eye_queue(void)
{
    return s_eye;
}

void ipc_publish_motion(bool wrist_still, const imu_sample_t *sample)
{
    if (xSemaphoreTake(s_imu_mu, pdMS_TO_TICKS(20)) != pdTRUE) {
        return;
    }
    s_wrist_still = wrist_still;
    if (sample) {
        s_sample = *sample;
        s_have_sample = true;
    }
    xSemaphoreGive(s_imu_mu);
}

bool ipc_wrist_still(imu_sample_t *sample_out)
{
    bool still = true;
    if (xSemaphoreTake(s_imu_mu, pdMS_TO_TICKS(20)) != pdTRUE) {
        return true;
    }
    still = s_wrist_still;
    if (sample_out && s_have_sample) {
        *sample_out = s_sample;
    }
    xSemaphoreGive(s_imu_mu);
    return still;
}
