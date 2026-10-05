/* rover.c - mission wiring: the one file that knows about every app.
 * Platform-independent; called by app_main() on the ESP32 and by main() in
 * the host build. To change the mission, change which apps start here. */
#include "rover.h"
#include "osal.h"
#include "sb.h"

#include "hs_app.h"
#include "tlm_app.h"
#include "cmd_app.h"
#include "bme688_app.h"
#include "sched_app.h"

#define TAG "ROVER"

void rover_start(void)
{
    if (SB_Init() != SB_OK) { OSAL_LOGE(TAG, "SB_Init failed"); return; }

    /* Subscribers first, scheduler (the tick source) last. */
    hs_task_start();
    tlm_task_start();
    cmd_task_start();
    bme688_task_start();
    sched_task_start();

    OSAL_LOGI(TAG, "flight software started");
}
