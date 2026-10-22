#include "ipc.h"

static QueueHandle_t s_sight;
static QueueHandle_t s_eye;

void ipc_init(void)
{
    s_sight = xQueueCreate(1, sizeof(sight_msg_t));
    s_eye = xQueueCreate(1, sizeof(eye_msg_t));
}

QueueHandle_t ipc_sight_queue(void)
{
    return s_sight;
}

QueueHandle_t ipc_eye_queue(void)
{
    return s_eye;
}
