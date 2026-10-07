/* osal_freertos.c - OSAL port for ESP-IDF / FreeRTOS (systems target). */
#include "osal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"

// todo: additional abstraction layer should be added for HW level, ie. GPIO, I2C...

struct osal_task  { TaskHandle_t h; osal_task_fn fn; void *arg; };
struct osal_queue { QueueHandle_t q; size_t item_size; };
struct osal_mutex { SemaphoreHandle_t m; };

/* ---- TIME ---- */

static TickType_t to_ticks(osal_ms_t t)
{
    return (t == OSAL_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(t);
}

osal_ms_t osal_time_ms(void)
{
    return (osal_ms_t)(esp_timer_get_time() / 1000);
}

/* ---- TASKS ---- */

static UBaseType_t map_prio(osal_prio_t p)
{
    switch (p) {
    case OSAL_PRIO_LOW:      return 2;
    case OSAL_PRIO_NORMAL:   return 3;
    case OSAL_PRIO_HIGH:     return 4;
    case OSAL_PRIO_CRITICAL: return 5;
    default:                 return 2;
    }
}

// compatibility wrapper for osal_task
static void trampoline(void *p)
{
    struct osal_task *t = p;
    t->fn(t->arg); // calls the osal task with osal arg
    vTaskDelete(NULL);
}

osal_status_t osal_task_create(osal_task_t *out, const osal_task_attr_t *attr, osal_task_fn fn, void *arg)
{
    if (!out || !attr || !fn) return OSAL_ERR_INVALID;

    struct osal_task *t = calloc(1, sizeof *t);
    if (!t) return OSAL_ERR_NOMEM;

    t->fn = fn; t->arg = arg;
    if (xTaskCreate(trampoline, attr->name, attr->stack_bytes, t,
                    map_prio(attr->priority), &t->h) != pdPASS) {
        free(t);
        return OSAL_ERR_NOMEM;
    }
    *out = t;
    return OSAL_OK;
}

void osal_task_exit(void) { vTaskDelete(NULL); }

void osal_task_delay_ms(osal_ms_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

void osal_task_delay_until(osal_ms_t *last_wake_ms, osal_ms_t period_ms)
{
    osal_ms_t target = *last_wake_ms + period_ms;
    int32_t remaining = (int32_t)(target - osal_time_ms());
    if (remaining > 0) osal_task_delay_ms((osal_ms_t)remaining);
    *last_wake_ms = target;
}

/* ---- QUEUES ---- */

osal_status_t osal_queue_create(osal_queue_t *out, size_t item_size, size_t depth)
{
    if (!out || !item_size || !depth) return OSAL_ERR_INVALID;
    struct osal_queue *q = calloc(1, sizeof *q);
    if (!q) return OSAL_ERR_NOMEM;
    q->q = xQueueCreate(depth, item_size);
    if (!q->q) { free(q); return OSAL_ERR_NOMEM; }
    q->item_size = item_size;
    *out = q;
    return OSAL_OK;
}

void osal_queue_delete(osal_queue_t q)
{
    if (!q) return;
    vQueueDelete(q->q);
    free(q);
}

osal_status_t osal_queue_send(osal_queue_t q, const void *item, osal_ms_t timeout)
{
    if (!q || !item) return OSAL_ERR_INVALID;
    if (xQueueSend(q->q, item, to_ticks(timeout)) == pdTRUE) return OSAL_OK;
    return (timeout == OSAL_NO_WAIT) ? OSAL_ERR_FULL : OSAL_ERR_TIMEOUT;
}

osal_status_t osal_queue_receive(osal_queue_t q, void *item, osal_ms_t timeout)
{
    if (!q || !item) return OSAL_ERR_INVALID;
    return (xQueueReceive(q->q, item, to_ticks(timeout)) == pdTRUE)
               ? OSAL_OK : OSAL_ERR_TIMEOUT;
}

size_t osal_queue_count(osal_queue_t q)
{
    return q ? (size_t)uxQueueMessagesWaiting(q->q) : 0;
}

/* ---- MUTEX ---- */

osal_status_t osal_mutex_create(osal_mutex_t *out)
{
    if (!out) return OSAL_ERR_INVALID;
    struct osal_mutex *m = calloc(1, sizeof *m);
    if (!m) return OSAL_ERR_NOMEM;
    m->m = xSemaphoreCreateMutex();
    if (!m->m) { free(m); return OSAL_ERR_NOMEM; }
    *out = m;
    return OSAL_OK;
}

void osal_mutex_delete(osal_mutex_t m)
{
    if (!m) return;
    vSemaphoreDelete(m->m);
    free(m);
}

osal_status_t osal_mutex_lock(osal_mutex_t m, osal_ms_t timeout)
{
    if (!m) return OSAL_ERR_INVALID;
    return (xSemaphoreTake(m->m, to_ticks(timeout)) == pdTRUE)
               ? OSAL_OK : OSAL_ERR_TIMEOUT;
}

void osal_mutex_unlock(osal_mutex_t m) { if (m) xSemaphoreGive(m->m); }

/* ---- LOGGING ---- */

void osal_log(osal_log_level_t level, const char *tag, const char *fmt, ...)
{
    char buf[192];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    switch (level) {
    case OSAL_LOG_ERROR: ESP_LOGE(tag, "%s", buf); break;
    case OSAL_LOG_WARN:  ESP_LOGW(tag, "%s", buf); break;
    case OSAL_LOG_INFO:  ESP_LOGI(tag, "%s", buf); break;
    default:             ESP_LOGD(tag, "%s", buf); break;
    }
}
