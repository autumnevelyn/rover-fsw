/* apid.h - single source of truth for all message IDs.
 * 0x000-0x7FF: CCSDS packet APIDs (11 bits) - these go on the wire.
 * 0xF00+     : internal bus topics - never leave the ESP32.
 * Reserved: 0x02x future navigation commands, 0x1xx Stage 2 instruments. */
#ifndef APID_H
#define APID_H

/* Telemetry (TM) */
#define APID_BME688_TLM          0x001
#define APID_HK_TLM              0x002
#define APID_CMD_ACK_TLM         0x003

/* Telecommands (TC) */
#define APID_CMD_SET_TLM_RATE    0x010
#define APID_CMD_REQ_READING     0x011
#define APID_CMD_SAFE_MODE       0x012

#define APID_IDLE                0x7FF

/* Internal topics */
#define APID_SCHED_TICK_1HZ      0xF01

#endif
