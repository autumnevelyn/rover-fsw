## Structure

```
osal/              platform abstraction (FreeRTOS + POSIX ports)
components/
  sb/              software bus (depends on osal)
  rover_msgs/      shared APIDs and message structs
  apps/
    app_sched/
    app_bme688/    sensor driver
    app_tlm/       telemetry packetizer
    app_cmd/       command ingest
    app_hs/        housekeeping; watchdog
main/              mission wiring (rover.c) + entry point (main.c)
host/              POSIX host build
```
