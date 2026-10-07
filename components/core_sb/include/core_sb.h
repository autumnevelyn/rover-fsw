/* core_sb.h - Software Bus: publish/subscribe message routing (cFS-style).
 * Apps never call each other; they subscribe to APIDs and publish to APIDs.
 * Built only on osal.h, so it runs unchanged on ESP-IDF and on the host. */
#ifndef SB_H
#define SB_H

#include <stdint.h>
#include <stddef.h>
#include "osal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_MAX_PAYLOAD         64   /* bytes per message                */
#define SB_MAX_ROUTES          32   /* distinct APIDs that can be routed */
#define SB_MAX_SUBS_PER_TOPIC  4

typedef enum {
    SB_OK = 0,
    SB_ERR_INVALID,      /* bad argument or message too long   */
    SB_ERR_NOT_INIT,
    SB_ERR_TABLE_FULL,   /* route table or subscriber list full */
    SB_ERR_NOT_FOUND     /* unsubscribe: no such subscription   */
} SB_Status_t;

/* One message as stored in a pipe. */
typedef struct {
    uint16_t apid;
    uint16_t len;
    uint8_t  data[SB_MAX_PAYLOAD];
} SB_Msg_t;

SB_Status_t SB_Init(void);

/* A pipe is a queue of SB_Msg_t owned by one subscribing task. */
SB_Status_t SB_PipeCreate(osal_queue_t *pipe, size_t depth);

SB_Status_t SB_Subscribe(uint16_t apid, osal_queue_t pipe);
SB_Status_t SB_Unsubscribe(uint16_t apid, osal_queue_t pipe);

/* Non-blocking fan-out: a full subscriber pipe drops the message (counted by
 * SB_GetDropCount) instead of stalling the publisher. Publishing to a topic
 * with no subscribers is not an error. */
SB_Status_t SB_Publish(uint16_t apid, const void *msg, size_t len);

/* Wait for the next message on a pipe. */
osal_status_t SB_Receive(osal_queue_t pipe, SB_Msg_t *out, osal_ms_t timeout);

uint32_t SB_GetDropCount(void);

#ifdef __cplusplus
}
#endif
#endif /* SB_H */
