#include "sb.h"
#include <string.h>

typedef struct {
    uint16_t     apid;
    osal_queue_t subs[SB_MAX_SUBS_PER_TOPIC];
    uint8_t      sub_count;
} SB_Route_t;

static SB_Route_t   s_routes[SB_MAX_ROUTES];
static size_t       s_route_count;
static osal_mutex_t s_lock;
static uint32_t     s_drops;
static bool         s_ready;

static SB_Route_t *find_route(uint16_t apid)
{
    for (size_t i = 0; i < s_route_count; i++)
        if (s_routes[i].apid == apid) return &s_routes[i];
    return NULL;
}

SB_Status_t SB_Init(void)
{
    if (s_ready) return SB_OK;
    memset(s_routes, 0, sizeof s_routes);
    s_route_count = 0;
    s_drops = 0;
    if (osal_mutex_create(&s_lock) != OSAL_OK) return SB_ERR_INVALID;
    s_ready = true;
    return SB_OK;
}

SB_Status_t SB_PipeCreate(osal_queue_t *pipe, size_t depth)
{
    if (!pipe || !depth) return SB_ERR_INVALID;
    return osal_queue_create(pipe, sizeof(SB_Msg_t), depth) == OSAL_OK
               ? SB_OK : SB_ERR_INVALID;
}

SB_Status_t SB_Subscribe(uint16_t apid, osal_queue_t pipe)
{
    if (!s_ready) return SB_ERR_NOT_INIT;
    if (!pipe) return SB_ERR_INVALID;
    osal_mutex_lock(s_lock, OSAL_WAIT_FOREVER);
    SB_Status_t st = SB_OK;
    SB_Route_t *r = find_route(apid);
    if (!r) {
        if (s_route_count == SB_MAX_ROUTES) st = SB_ERR_TABLE_FULL;
        else { r = &s_routes[s_route_count++]; r->apid = apid; }
    }
    if (st == SB_OK) {
        bool dup = false;
        for (uint8_t i = 0; i < r->sub_count; i++) dup |= (r->subs[i] == pipe);
        if (!dup) {
            if (r->sub_count == SB_MAX_SUBS_PER_TOPIC) st = SB_ERR_TABLE_FULL;
            else r->subs[r->sub_count++] = pipe;
        }
    }
    osal_mutex_unlock(s_lock);
    return st;
}

SB_Status_t SB_Unsubscribe(uint16_t apid, osal_queue_t pipe)
{
    if (!s_ready) return SB_ERR_NOT_INIT;
    SB_Status_t st = SB_ERR_NOT_FOUND;
    osal_mutex_lock(s_lock, OSAL_WAIT_FOREVER);
    SB_Route_t *r = find_route(apid);
    for (uint8_t i = 0; r && i < r->sub_count; i++) {
        if (r->subs[i] == pipe) {
            r->subs[i] = r->subs[--r->sub_count];
            st = SB_OK;
            break;
        }
    }
    osal_mutex_unlock(s_lock);
    return st;
}

SB_Status_t SB_Publish(uint16_t apid, const void *msg, size_t len)
{
    if (!s_ready) return SB_ERR_NOT_INIT;
    if (len > SB_MAX_PAYLOAD || (len && !msg)) return SB_ERR_INVALID;

    SB_Msg_t m = { .apid = apid, .len = (uint16_t)len };
    if (len) memcpy(m.data, msg, len);

    osal_mutex_lock(s_lock, OSAL_WAIT_FOREVER);
    SB_Route_t *r = find_route(apid);
    for (uint8_t i = 0; r && i < r->sub_count; i++) {
        /* OSAL_NO_WAIT: never block while holding the bus lock. */
        if (osal_queue_send(r->subs[i], &m, OSAL_NO_WAIT) != OSAL_OK) s_drops++;
    }
    osal_mutex_unlock(s_lock);
    return SB_OK;
}

osal_status_t SB_Receive(osal_queue_t pipe, SB_Msg_t *out, osal_ms_t timeout)
{
    return osal_queue_receive(pipe, out, timeout);
}

uint32_t SB_GetDropCount(void)
{
    if (!s_ready) return 0;
    osal_mutex_lock(s_lock, OSAL_WAIT_FOREVER);
    uint32_t d = s_drops;
    osal_mutex_unlock(s_lock);
    return d;
}
