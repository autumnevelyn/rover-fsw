/* Telemetry Manager. STUB: logs what it receives. Waiting on the CCSDS +
 * CRC-16 + COBS library (build step 2): then it builds packets and writes
 * them to the UART. */
#include "tlm_app.h"
#include "apid.h"
#include "msgs.h"
#include "core_sb.h"
#include <string.h>

#define TAG "TLM"

static osal_queue_t s_pipe;

static void tlm_task(void *arg)
{
    (void)arg;
    SB_Msg_t m;
    for (;;) {
        if (SB_Receive(s_pipe, &m, OSAL_WAIT_FOREVER) != OSAL_OK) continue;
        /* TODO: ccsds_build_tm(apid, seq, payload) -> crc16 -> cobs_encode -> UART */
        if (m.apid == APID_BME688_TLM && m.len == sizeof(bme688_tlm_t)) {
            bme688_tlm_t t;
            memcpy(&t, m.data, sizeof t);
            OSAL_LOGI(TAG, "BME688 T=%.1fC H=%.1f%% P=%.1fhPa gas=%.0f",
                      t.temperature_c, t.humidity_pct, t.pressure_hpa,
                      t.gas_resistance_ohm);
        } else if (m.apid == APID_HK_TLM && m.len == sizeof(hk_tlm_t)) {
            hk_tlm_t h;
            memcpy(&h, m.data, sizeof h);
            OSAL_LOGI(TAG, "HK up=%us frame=%u drops=%u safe=%u",
                      (unsigned)h.uptime_s, (unsigned)h.frame,
                      (unsigned)h.sb_drops, (unsigned)h.safe_mode);
        } else {
            OSAL_LOGI(TAG, "TM apid=0x%03X len=%u", (unsigned)m.apid, (unsigned)m.len);
        }
    }
}

osal_status_t tlm_task_start(void)
{
    static const osal_task_attr_t attr = { "tlm", 4096, OSAL_PRIO_NORMAL };
    if (SB_PipeCreate(&s_pipe, 16) != SB_OK) return OSAL_ERR_NOMEM;
    SB_Subscribe(APID_BME688_TLM, s_pipe);
    SB_Subscribe(APID_HK_TLM, s_pipe);
    SB_Subscribe(APID_CMD_ACK_TLM, s_pipe);
    osal_task_t t;
    osal_status_t st = osal_task_create(&t, &attr, tlm_task, NULL);
    if (st != OSAL_OK) OSAL_LOGE(TAG, "task create failed");
    return st;
}
