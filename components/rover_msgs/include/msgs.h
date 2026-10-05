/* msgs.h - payload layouts carried on the bus (host byte order, internal). */
#ifndef MSGS_H
#define MSGS_H
#include <stdint.h>

typedef struct { uint32_t frame; } sched_tick_msg_t;

/* 16 bytes - becomes the CCSDS data field of APID 0x001. */
typedef struct {
    float temperature_c;
    float humidity_pct;
    float pressure_hpa;
    float gas_resistance_ohm;
} bme688_tlm_t;

typedef struct {
    uint32_t uptime_s;
    uint32_t frame;
    uint32_t sb_drops;
    uint8_t  safe_mode;
} hk_tlm_t;

#endif
