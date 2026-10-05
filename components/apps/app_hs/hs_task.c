/* Health & Safety. Publishes housekeeping telemetry each tick.
 * TODO: hardware watchdog feed (needs hal_wdt), fault detection, safe mode. */
#include "hs_app.h"
#include "apid.h"
#include "msgs.h"
#include "sb.h"

#define TAG "H&S"

static osal_queue_t s_pipe;

static void hs_task(void *arg)
{
    (void)arg;
    hk_tlm_t hk = { 0 };
    SB_Msg_t m;
    for (;;) {
        if (SB_Receive(s_pipe, &m, OSAL_WAIT_FOREVER) != OSAL_OK) continue;
        if (m.apid != APID_SCHED_TICK_1HZ) continue;
        /* TODO: feed watchdog; check task heartbeats; enter safe mode on fault. */
        hk.frame++;
        hk.uptime_s  = osal_time_ms() / 1000;
        hk.sb_drops  = SB_GetDropCount();
        SB_Publish(APID_HK_TLM, &hk, sizeof hk);
    }
}

osal_status_t hs_task_start(void)
{
    static const osal_task_attr_t attr = { "hs", 4096, OSAL_PRIO_CRITICAL };
    if (SB_PipeCreate(&s_pipe, 8) != SB_OK) return OSAL_ERR_NOMEM;
    SB_Subscribe(APID_SCHED_TICK_1HZ, s_pipe);
    osal_task_t t;
    osal_status_t st = osal_task_create(&t, &attr, hs_task, NULL);
    if (st != OSAL_OK) OSAL_LOGE(TAG, "task create failed");
    return st;
}
