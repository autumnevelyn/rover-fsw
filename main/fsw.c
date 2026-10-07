/* fsw.c - system wiring: the one file that knows about every app.
 * Platform-independent; called by app_main() on the ESP32 and by main() in
 * the host build. Apps can be disabled or reordered from here. */
#include "fsw.h"
#include "osal.h"
#include "core_sb.h"

#include "hs_app.h"
#include "tlm_app.h"
#include "cmd_app.h"
#include "bme688_app.h"
#include "sched_app.h"

#define TAG "FSW"

void fsw_start(void)
{
    if (SB_Init() != SB_OK) { OSAL_LOGE(TAG, "SB_Init failed"); return; }

    // subscribers first, scheduler (tick source) last.
    hs_task_start();
    tlm_task_start();
    cmd_task_start();
    bme688_task_start();
    sched_task_start();

    OSAL_LOGI(TAG, "flight software started");
}
