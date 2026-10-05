/* sched_app.h - public interface of the sched app (start function only). */
#ifndef SCHED_APP_H
#define SCHED_APP_H
#include "osal.h"

/* Creates the app's pipe, subscribes it, then spawns its task. */
osal_status_t sched_task_start(void);

#endif
