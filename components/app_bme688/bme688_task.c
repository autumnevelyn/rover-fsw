/* BME688 driver app. STUB: publishes synthetic readings until the real I2C
 * driver exists (needs the future hardware layer). Runs on tick or on an
 * "immediate reading" command. */
#include "bme688_app.h"
#include "apid.h"
#include "msgs.h"
#include "core_sb.h"

#define TAG "BME"

static osal_queue_t s_pipe;

static void sample_and_publish(uint32_t n)
{
    /* TODO: replace with real sensor read via hal_i2c. */
    bme688_tlm_t t = {
        .temperature_c      = 22.4f + 0.1f * (float)(n % 10),
        .humidity_pct       = 41.2f,
        .pressure_hpa       = 1013.2f,
        .gas_resistance_ohm = 88500.0f,
    };
    SB_Publish(APID_BME688_TLM, &t, sizeof t);
}

static void bme688_task(void *arg)
{
    (void)arg;
    uint32_t n = 0;
    SB_Msg_t m;
    for (;;) {
        if (SB_Receive(s_pipe, &m, OSAL_WAIT_FOREVER) != OSAL_OK) continue;
        if (m.apid == APID_SCHED_TICK_1HZ || m.apid == APID_CMD_REQ_READING)
            sample_and_publish(n++);
    }
}

osal_status_t bme688_task_start(void)
{
    static const osal_task_attr_t attr = { "bme688", 4096, OSAL_PRIO_LOW };
    if (SB_PipeCreate(&s_pipe, 8) != SB_OK) return OSAL_ERR_NOMEM;
    SB_Subscribe(APID_SCHED_TICK_1HZ, s_pipe);
    SB_Subscribe(APID_CMD_REQ_READING, s_pipe);
    osal_task_t t;
    osal_status_t st = osal_task_create(&t, &attr, bme688_task, NULL);
    if (st != OSAL_OK) OSAL_LOGE(TAG, "task create failed");
    return st;
}
