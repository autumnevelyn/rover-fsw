/* cmd_app.h - public interface of the cmd app (start function only). */
#ifndef CMD_APP_H
#define CMD_APP_H
#include "osal.h"

/* Creates the app's pipe, subscribes it, then spawns its task. */
osal_status_t cmd_task_start(void);

#endif
