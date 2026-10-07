## Structure

```
osal/             platform abstraction (FreeRTOS + POSIX ports)
components/
  app_sched/
  app_bme688/     sensor driver
  app_tlm/        telemetry packetizer
  app_cmd/        command ingest
  app_hs/         housekeeping; watchdog
  core_sb/        software bus (depends on osal)
  core_msgs/      shared APIDs and message structs
main/             mission wiring (fsw.c) + entry point (main.c)
host/             POSIX host build
```

## Build
- Only CmakeLists files are IDF-specific. Host build uses a Makefile