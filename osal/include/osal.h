/*
 * osal.h - Rover OS Abstraction Layer (Stage 1 minimum)
 *
 * Rules:
 *  - Apps include ONLY this header, never FreeRTOS/ESP-IDF/pthread headers.
 *  - No platform types in the API: all handles are opaque, all results are
 *    osal_status_t, all times are milliseconds.
 *  - Only add a function when a task actually needs it.
 *
 * Ports (one per platform, same header):
 *  - osal_freertos.c : ESP-IDF / FreeRTOS (flight target)
 *  - osal_posix.c    : Linux/macOS host build (pthreads) for testing
 */
#ifndef OSAL_H
#define OSAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- Status & timeouts ---------- */

typedef enum {
    OSAL_OK = 0,
    OSAL_ERR_TIMEOUT,   /* operation did not finish within the timeout   */
    OSAL_ERR_FULL,      /* queue full (send with OSAL_NO_WAIT)           */
    OSAL_ERR_INVALID,   /* bad argument / handle                         */
    OSAL_ERR_NOMEM,     /* allocation failed                             */
    OSAL_ERR_GENERIC    /* anything else; port should log the cause      */
} osal_status_t;

typedef uint32_t osal_ms_t;
#define OSAL_NO_WAIT       ((osal_ms_t)0)
#define OSAL_WAIT_FOREVER  ((osal_ms_t)0xFFFFFFFFu)

/* ---------- Time ---------- */

/* Monotonic milliseconds since boot. Wraps after ~49 days; compare with
 * subtraction (a - b), never with < or >. */
osal_ms_t osal_time_ms(void);

/* ---------- Tasks ---------- */

/* Abstract priority levels; each port maps them to native priorities. */
typedef enum {
    OSAL_PRIO_LOW = 0,      /* e.g. science sensor drivers          */
    OSAL_PRIO_NORMAL,       /* e.g. TLM, CMD                        */
    OSAL_PRIO_HIGH,         /* e.g. Scheduler                       */
    OSAL_PRIO_CRITICAL      /* e.g. Health & Safety                 */
} osal_prio_t;

typedef struct osal_task *osal_task_t;          /* opaque handle */
typedef void (*osal_task_fn)(void *arg);

typedef struct {
    const char *name;
    size_t      stack_bytes;
    osal_prio_t priority;
} osal_task_attr_t;

/* The task function must never return; call osal_task_exit() to end it. */
osal_status_t osal_task_create(osal_task_t *out, const osal_task_attr_t *attr,
                               osal_task_fn fn, void *arg);
void          osal_task_exit(void);

/* Relative sleep. */
void osal_task_delay_ms(osal_ms_t ms);

/* Drift-free periodic wake: sleeps until (*last_wake_ms + period_ms), then
 * advances *last_wake_ms by period_ms. If the deadline already passed it
 * returns immediately. Initialise *last_wake_ms = osal_time_ms() once. */
void osal_task_delay_until(osal_ms_t *last_wake_ms, osal_ms_t period_ms);

/* ---------- Queues (copy-by-value; the software bus is built on these) ---- */

typedef struct osal_queue *osal_queue_t;        /* opaque handle */

osal_status_t osal_queue_create(osal_queue_t *out, size_t item_size,
                                size_t depth);
void          osal_queue_delete(osal_queue_t q);

/* Copies item_size bytes in. With OSAL_NO_WAIT returns OSAL_ERR_FULL instead
 * of blocking - this is what gives the bus its drop-don't-stall fan-out. */
osal_status_t osal_queue_send(osal_queue_t q, const void *item,
                              osal_ms_t timeout);

/* Copies item_size bytes out. Returns OSAL_ERR_TIMEOUT if nothing arrived. */
osal_status_t osal_queue_receive(osal_queue_t q, void *item,
                                 osal_ms_t timeout);

size_t osal_queue_count(osal_queue_t q);        /* items waiting */

/* ---------- Mutex (non-recursive) ---------- */

typedef struct osal_mutex *osal_mutex_t;        /* opaque handle */

osal_status_t osal_mutex_create(osal_mutex_t *out);
void          osal_mutex_delete(osal_mutex_t m);
osal_status_t osal_mutex_lock(osal_mutex_t m, osal_ms_t timeout);
void          osal_mutex_unlock(osal_mutex_t m);

/* ---------- Logging ---------- */

typedef enum {
    OSAL_LOG_ERROR = 0,
    OSAL_LOG_WARN,
    OSAL_LOG_INFO,
    OSAL_LOG_DEBUG
} osal_log_level_t;

void osal_log(osal_log_level_t level, const char *tag, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

#define OSAL_LOGE(tag, ...) osal_log(OSAL_LOG_ERROR, tag, __VA_ARGS__)
#define OSAL_LOGW(tag, ...) osal_log(OSAL_LOG_WARN,  tag, __VA_ARGS__)
#define OSAL_LOGI(tag, ...) osal_log(OSAL_LOG_INFO,  tag, __VA_ARGS__)
#define OSAL_LOGD(tag, ...) osal_log(OSAL_LOG_DEBUG, tag, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* OSAL_H */
