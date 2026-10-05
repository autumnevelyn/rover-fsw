/* bme688_app.h - public interface of the bme688 app (start function only). */
#ifndef BME688_APP_H
#define BME688_APP_H
#include "osal.h"

/* Creates the app's pipe, subscribes it, then spawns its task. */
osal_status_t bme688_task_start(void);

#endif
