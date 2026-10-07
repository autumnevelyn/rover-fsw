/* Host entry point: runs the flight software on a PC for N seconds. */
#include "fsw.h"
#include "osal.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int seconds = (argc > 1) ? atoi(argv[1]) : 5;
    fsw_start();
    osal_task_delay_ms((osal_ms_t)seconds * 1000);
    fprintf(stderr, "host run finished\n");
    return 0;
}
