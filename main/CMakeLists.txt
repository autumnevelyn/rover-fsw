idf_component_register(SRCS "main.c" "rover.c"
                       INCLUDE_DIRS "."
                       REQUIRES osal sb app_sched app_bme688 app_tlm app_cmd app_hs)
