/* Scheduler: drives the 1 Hz major frame. Other apps run off its tick. */
#include "sched_app.h"
#include "apid.h"
#include "msgs.h"
#include "sb.h"

#define TAG "SCH"
#define MAJOR_FRAME_MS 1000

static void sched_task(void *arg)
{
    (void)arg;
    osal_ms_t last = osal_time_ms();
    sched_tick_msg_t tick = { 0 };
    for (;;) {
        osal_task_delay_until(&last, MAJOR_FRAME_MS);
        tick.frame++;
        SB_Publish(APID_SCHED_TICK_1HZ, &tick, sizeof tick);
    }
}

osal_status_t sched_task_start(void)
{
    static const osal_task_attr_t attr = { "sched", 4096, OSAL_PRIO_HIGH };
    osal_task_t t;
    osal_status_t st = osal_task_create(&t, &attr, sched_task, NULL);
    if (st != OSAL_OK) OSAL_LOGE(TAG, "task create failed");
    return st;
}
