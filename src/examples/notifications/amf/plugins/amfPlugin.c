/*
 * An example AMF plugin to faster node detect status
 */
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <clAmfPluginApi.h>
#include <sys/inotify.h>

// This structure defines the plugin's interface with the program that loads it.
ClAmfPluginApi api;

#define NODESTATUS "/tmp/nodeStatus.txt"

static ClNodeStatus nodeMap[1024];
static unsigned char buffer[1024];

ClNodeStatus clAmfPluginStatus(ClIocNodeAddressT node)
{
    return nodeMap[node];
}

void updateANotificationNodeStatus()
{
    std::string strRowStat;
    std::fstream statFile(NODESTATUS, std::ios::in);

    if (statFile)
    {
        getline(statFile, strRowStat);
        while (statFile) {
            std::stringstream ssRowStat(strRowStat);
            std::string status;
            int nodeNum;

            ssRowStat >> nodeNum >> status;
            if (status.compare("down") == 0)
            {
                // Check to call passed notification func about node status change
                if (nodeMap[nodeNum] == ClNodeStatusUp)
                {
                    if (api.clAmfNotificationCallback)
                        api.clAmfNotificationCallback(nodeNum, ClNodeStatusDown);
                }
                nodeMap[nodeNum] = ClNodeStatusDown;
            }
            else if (status.compare("up") == 0)
            {
                // Check to call passed notification func about node status change
                if (nodeMap[nodeNum] == ClNodeStatusDown)
                {
                    if (api.clAmfNotificationCallback)
                        api.clAmfNotificationCallback(nodeNum, ClNodeStatusUp);
                }
                nodeMap[nodeNum] = ClNodeStatusUp;
            }
            else
            {
                nodeMap[nodeNum] = ClNodeStatusUnknown;
            }
            getline(statFile, strRowStat);
        }
        statFile.close();
    }
}

void *clPluginRunTask(void *pParam)
{
    int inotifyFd, wd;
    ssize_t numRead;

    int fd = open(NODESTATUS, O_RDWR | O_CREAT, 0777);

    if (fd < 0)
    {
        perror("open!");
        return 0;
    }

    memset(&nodeMap, 0, sizeof(nodeMap));
    updateANotificationNodeStatus();

    inotifyFd = inotify_init(); /* Create inotify instance */
    if (inotifyFd == -1)
        perror("inotify_init!");

    /* For each command-line argument, add a watch for all events */
    wd = inotify_add_watch(inotifyFd, NODESTATUS, IN_MODIFY);
    if (wd == -1)
        perror("inotify_add_watch");

    for (;;)
    {
        /* Read events forever */
        numRead = read(inotifyFd, buffer, sizeof(buffer));
        if (numRead == 0)
            perror("read() from inotify fd returned 0!");

        if (numRead == -1)
            perror("read");

        updateANotificationNodeStatus();
    }

    close(wd);
    close(inotifyFd);
    close(fd);
    return EXIT_SUCCESS;
}

ClRcT clAmfPluginNotificationRun(clAmfPluginNotificationCallbackT callback)
{
    if (callback)
        api.clAmfNotificationCallback = callback;

    clLogInfo("AMF", "PLG", "An example AMF plugin");
    /*
     * main loop here...
     */
    ClRcT rc = clOsalTaskCreateDetached("PluginRunTask", CL_OSAL_SCHED_OTHER, CL_OSAL_THREAD_PRI_NOT_APPLICABLE, 0, clPluginRunTask, NULL);
    return rc;
}

ClPlugin *clPluginInitialize(ClWordT preferredPluginVersion)
{
    // We can only provide a single version, so don't bother with the 'preferredPluginVersion' variable.

    // Initialize the plugin data structure
    api.pluginInfo.pluginId = CL_AMF_PLUGIN_ID;
    api.pluginInfo.pluginVersion = CL_AMF_PLUGIN_VERSION;
    api.clAmfRunTask = clAmfPluginNotificationRun;
    api.clAmfNodeStatus = clAmfPluginStatus;

    // return it
    return (ClPlugin*) &api;
}

int main(int argc, char *agrv[])
{
    /*
     * Monitor /tmp/nodestatus.txt
     * Read file and notify the status of each node
     * 1 up
     * 2 down
     * 3 up
     * 4 up
     * 5 down
     * 6 up
     * 7 up
     * 8 down
     * 9 unknown
     * 10 unknown
     */
    clPluginRunTask(NULL);
    return CL_OK;
}

