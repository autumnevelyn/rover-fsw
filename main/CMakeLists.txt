idf_component_register(SRCS "main.c" "fsw.c"
                       INCLUDE_DIRS "."
                       REQUIRES osal core_sb app_sched app_bme688 app_tlm app_cmd app_hs)
