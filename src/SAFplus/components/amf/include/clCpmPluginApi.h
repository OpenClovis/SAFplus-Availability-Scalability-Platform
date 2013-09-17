#ifndef CLCPMPLUGINAPI_H
#define CLCPMPLUGINAPI_H

#include <clPluginApi.h>
#include <clDebugApi.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define CL_CPM_NODE_DETECT_STATUS_PLUGIN_VERSION 1
#define CL_CPM_NODE_DETECT_STATUS_PLUGIN_ID 0x201309091029
#define CL_CPM_NODE_DETECT_STATUS_PLUGIN_NAME "cpmStatusNodeGet.so"
#define CL_CPM_PLUGIN_RUN_TASK_NAME "clPluginNotificationRun"

typedef enum
{
    ClNodeStatusDown,
    ClNodeStatusUp,
    ClNodeStatusUnknown,  // plugin cannot determine the status of this node.
} ClNodeStatus;

typedef void (*clCpmNotificationCallbackT)(ClIocNodeAddressT node, ClNodeStatus status);

/*
 * Function loop forever
 */
typedef int (*clPluginNotificationRunT)(clCpmNotificationCallbackT callback);

typedef struct
{
    ClPlugin pluginInfo;
    clCpmNotificationCallbackT callback;
} ClCpmPluginApi;

#ifdef __cplusplus
}
#endif
#endif
