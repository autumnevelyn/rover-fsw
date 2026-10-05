/* Command Ingest. STUB: idles. Waiting on the CCSDS/CRC/COBS library and the
 * UART layer. Final flow: read UART -> COBS decode -> CRC check -> sequence
 * check -> APID whitelist -> SB_Publish, plus an ack/nak on APID_CMD_ACK_TLM. */
#include "cmd_app.h"
#include "apid.h"
#include "sb.h"

#define TAG "CMD"

static void cmd_task(void *arg)
{
    (void)arg;
    for (;;) {
        /* TODO: blocking UART read + parse + dispatch. */
        osal_task_delay_ms(100);
    }
}

osal_status_t cmd_task_start(void)
{
    static const osal_task_attr_t attr = { "cmd", 4096, OSAL_PRIO_NORMAL };
    osal_task_t t;
    osal_status_t st = osal_task_create(&t, &attr, cmd_task, NULL);
    if (st != OSAL_OK) OSAL_LOGE(TAG, "task create failed");
    return st;
}
