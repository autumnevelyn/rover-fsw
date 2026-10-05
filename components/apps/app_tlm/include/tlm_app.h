/* tlm_app.h - public interface of the tlm app (start function only). */
#ifndef TLM_APP_H
#define TLM_APP_H
#include "osal.h"

/* Creates the app's pipe, subscribes it, then spawns its task. */
osal_status_t tlm_task_start(void);

#endif
