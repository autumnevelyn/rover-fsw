/* osal_posix.c - OSAL port for Linux/macOS host builds (pthreads).
 * Used to run and test the flight software without hardware.
 * Task priorities are ignored on the host. */
#define _GNU_SOURCE
#include "osal.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct osal_task  { pthread_t th; osal_task_fn fn; void *arg; };
struct osal_mutex { pthread_mutex_t m; };
struct osal_queue {
    pthread_mutex_t m;
    pthread_cond_t  not_empty, not_full;
    uint8_t *buf;
    size_t item_size, depth, head, count;
};

static void deadline(struct timespec *ts, clockid_t clk, osal_ms_t ms)
{
    clock_gettime(clk, ts);
    ts->tv_sec  += ms / 1000;
    ts->tv_nsec += (long)(ms % 1000) * 1000000L;
    if (ts->tv_nsec >= 1000000000L) { ts->tv_sec++; ts->tv_nsec -= 1000000000L; }
}

/* ---- time ---- */
osal_ms_t osal_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (osal_ms_t)((uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u);
}

/* ---- tasks ---- */
static void *trampoline(void *p)
{
    struct osal_task *t = p;
    t->fn(t->arg);
    return NULL;
}

osal_status_t osal_task_create(osal_task_t *out, const osal_task_attr_t *attr,
                               osal_task_fn fn, void *arg)
{
    if (!out || !attr || !fn) return OSAL_ERR_INVALID;
    struct osal_task *t = calloc(1, sizeof *t);
    if (!t) return OSAL_ERR_NOMEM;
    t->fn = fn; t->arg = arg;
    pthread_attr_t pa;
    pthread_attr_init(&pa);
    pthread_attr_setstacksize(&pa, attr->stack_bytes < 65536 ? 65536 : attr->stack_bytes);
    pthread_attr_setdetachstate(&pa, PTHREAD_CREATE_DETACHED);
    int rc = pthread_create(&t->th, &pa, trampoline, t);
    pthread_attr_destroy(&pa);
    if (rc) { free(t); return OSAL_ERR_NOMEM; }
    *out = t;
    return OSAL_OK;
}

void osal_task_exit(void) { pthread_exit(NULL); }

void osal_task_delay_ms(osal_ms_t ms)
{
    struct timespec ts = { ms / 1000, (long)(ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

void osal_task_delay_until(osal_ms_t *last_wake_ms, osal_ms_t period_ms)
{
    osal_ms_t target = *last_wake_ms + period_ms;
    int32_t remaining = (int32_t)(target - osal_time_ms());
    if (remaining > 0) osal_task_delay_ms((osal_ms_t)remaining);
    *last_wake_ms = target;
}

/* ---- queues ---- */
osal_status_t osal_queue_create(osal_queue_t *out, size_t item_size, size_t depth)
{
    if (!out || !item_size || !depth) return OSAL_ERR_INVALID;
    struct osal_queue *q = calloc(1, sizeof *q);
    if (!q) return OSAL_ERR_NOMEM;
    q->buf = malloc(item_size * depth);
    if (!q->buf) { free(q); return OSAL_ERR_NOMEM; }
    q->item_size = item_size; q->depth = depth;
    pthread_mutex_init(&q->m, NULL);
    pthread_condattr_t ca;
    pthread_condattr_init(&ca);
    pthread_condattr_setclock(&ca, CLOCK_MONOTONIC);
    pthread_cond_init(&q->not_empty, &ca);
    pthread_cond_init(&q->not_full, &ca);
    pthread_condattr_destroy(&ca);
    *out = q;
    return OSAL_OK;
}

void osal_queue_delete(osal_queue_t q)
{
    if (!q) return;
    pthread_mutex_destroy(&q->m);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
    free(q->buf);
    free(q);
}

static bool is_full(osal_queue_t q)  { return q->count == q->depth; }
static bool is_empty(osal_queue_t q) { return q->count == 0; }

/* Called with q->m held. Waits until blocked() is false or the timeout expires. */
static bool wait_while(osal_queue_t q, pthread_cond_t *cv, bool (*blocked)(osal_queue_t),
                       osal_ms_t timeout)
{
    struct timespec ts;
    if (timeout != OSAL_WAIT_FOREVER && timeout != OSAL_NO_WAIT)
        deadline(&ts, CLOCK_MONOTONIC, timeout);
    while (blocked(q)) {
        if (timeout == OSAL_NO_WAIT) return false;
        if (timeout == OSAL_WAIT_FOREVER) pthread_cond_wait(cv, &q->m);
        else if (pthread_cond_timedwait(cv, &q->m, &ts) == ETIMEDOUT && blocked(q))
            return false;
    }
    return true;
}

osal_status_t osal_queue_send(osal_queue_t q, const void *item, osal_ms_t timeout)
{
    if (!q || !item) return OSAL_ERR_INVALID;
    pthread_mutex_lock(&q->m);
    if (!wait_while(q, &q->not_full, is_full, timeout)) {
        pthread_mutex_unlock(&q->m);
        return (timeout == OSAL_NO_WAIT) ? OSAL_ERR_FULL : OSAL_ERR_TIMEOUT;
    }
    size_t tail = (q->head + q->count) % q->depth;
    memcpy(q->buf + tail * q->item_size, item, q->item_size);
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->m);
    return OSAL_OK;
}

osal_status_t osal_queue_receive(osal_queue_t q, void *item, osal_ms_t timeout)
{
    if (!q || !item) return OSAL_ERR_INVALID;
    pthread_mutex_lock(&q->m);
    if (!wait_while(q, &q->not_empty, is_empty, timeout)) {
        pthread_mutex_unlock(&q->m);
        return OSAL_ERR_TIMEOUT;
    }
    memcpy(item, q->buf + q->head * q->item_size, q->item_size);
    q->head = (q->head + 1) % q->depth;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->m);
    return OSAL_OK;
}

size_t osal_queue_count(osal_queue_t q)
{
    if (!q) return 0;
    pthread_mutex_lock(&q->m);
    size_t n = q->count;
    pthread_mutex_unlock(&q->m);
    return n;
}

/* ---- mutex ---- */
osal_status_t osal_mutex_create(osal_mutex_t *out)
{
    if (!out) return OSAL_ERR_INVALID;
    struct osal_mutex *m = calloc(1, sizeof *m);
    if (!m) return OSAL_ERR_NOMEM;
    pthread_mutex_init(&m->m, NULL);
    *out = m;
    return OSAL_OK;
}

void osal_mutex_delete(osal_mutex_t m)
{
    if (!m) return;
    pthread_mutex_destroy(&m->m);
    free(m);
}

osal_status_t osal_mutex_lock(osal_mutex_t m, osal_ms_t timeout)
{
    if (!m) return OSAL_ERR_INVALID;
    int rc;
    if (timeout == OSAL_WAIT_FOREVER) rc = pthread_mutex_lock(&m->m);
    else if (timeout == OSAL_NO_WAIT) rc = pthread_mutex_trylock(&m->m);
    else {
        struct timespec ts;
        deadline(&ts, CLOCK_REALTIME, timeout);
        rc = pthread_mutex_timedlock(&m->m, &ts);
    }
    return rc == 0 ? OSAL_OK : OSAL_ERR_TIMEOUT;
}

void osal_mutex_unlock(osal_mutex_t m) { if (m) pthread_mutex_unlock(&m->m); }

/* ---- logging ---- */
void osal_log(osal_log_level_t level, const char *tag, const char *fmt, ...)
{
    static const char lv[] = "EWID";
    fprintf(stderr, "%c (%u) %s: ", lv[level], (unsigned)osal_time_ms(), tag);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}
