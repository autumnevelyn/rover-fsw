/* hs_app.h - public interface of the hs app (start function only). */
#ifndef HS_APP_H
#define HS_APP_H
#include "osal.h"

/* Creates the app's pipe, subscribes it, then spawns its task. */
osal_status_t hs_task_start(void);

#endif
