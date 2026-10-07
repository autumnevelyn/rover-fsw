# Host (PC) build of the flight software using the POSIX OSAL port.
#   make        build
#   make run    build and run for 5 s
CC      ?= gcc
APPS    := sched bme688 tlm cmd hs
INCS    := -I../osal/include -I../components/core_sb/include \
           -I../components/core_msgs/include -I../main \
           $(foreach a,$(APPS),-I../components/apps/app_$(a)/include)
CFLAGS  := -std=c11 -Wall -Wextra -g -pthread $(INCS)
SRCS    := ../osal/ports/osal_posix.c ../components/core_sb/core_sb.c \
           $(foreach a,$(APPS),../components/apps/app_$(a)/$(a)_task.c) \
           ../main/fsw.c host_main.c

fsw_host: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $@

run: fsw_host
	./fsw_host 5

clean:
	rm -f fsw_host
.PHONY: run clean
