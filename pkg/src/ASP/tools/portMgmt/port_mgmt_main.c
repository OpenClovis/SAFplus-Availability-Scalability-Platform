#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "port_mgmt.h"

#ifdef PORT_MGMT_POLL

static int alarm_raised = 0;
static void alarm_handler(int sig)
{
    alarm_raised = 1;
}
static int if_notify_callback(const char *name, int status)
{
    port_mgmt_log("Interface [%s] status [%s]\n", name, status ? "UP" : "DOWN");
    return 0;
}

int main(int argc, char **argv)
{
    port_mgmt_initialize(if_notify_callback, 3);
    signal(SIGALRM, alarm_handler);
    alarm(60);
    while(!alarm_raised)
    {
        sleep(3);
    }
    return 0;
}

#else

int main(int argc, char **argv)
{
    struct port_interface_stats stats = {0};
    port_mgmt_refresh();
    if(argc > 1)
    {
        if(port_mgmt_get_stats(argv[1], &stats)==0)
        {
            port_mgmt_log("\n------------Interface stats for [%s]---------\n", argv[1]);
            port_mgmt_stats_dump(&stats);
        }
    }
    else
    {
        struct port_mgmt_info *info;
        int entries = 0;
        register int i;
        if(port_mgmt_list_info(&info, &entries))
        {
            port_mgmt_log("\nUnable to retrieve port mgmt info for [%s] interfaces", "all");
            return -1;
        }
        for(i = 0; i < entries; ++i)
        {
            port_mgmt_log("\n\nInterface stats [%s], index [%d], flags [%s], virtual [%d]\n",
                          info[i].name, info[i].index, info[i].flags & (IFF_UP|IFF_RUNNING) ? "UP" :"DOWN",
                          info[i].virtual);
            port_mgmt_stats_dump(&info[i].stats);
        }
        free(info);
    }
    return 0;
}

#endif
