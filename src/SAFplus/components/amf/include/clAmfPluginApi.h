#ifndef CL_AMF_PLUGIN_API_H
#define CL_AMF_PLUGIN_API_H

#include <clPluginApi.h>
#include <clDebugApi.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define CL_AMF_PLUGIN_VERSION 1
#define CL_AMF_PLUGIN_ID 0x201309091029
#define CL_AMF_PLUGIN_NAME "amfPlugin.so"

typedef enum
{
    ClNodeStatusDown,
    ClNodeStatusUp,
    ClNodeStatusUnknown,  // plugin cannot determine the status of this node.
} ClNodeStatus;

typedef void (*clAmfPluginNotificationCallbackT)(ClIocNodeAddressT node, ClNodeStatus status);

/*
 * Function loop forever
 */
typedef ClRcT (*clAmfPluginNotificationRunT)(clAmfPluginNotificationCallbackT callback);

typedef ClNodeStatus (*clAmfPluginNodeStatusT)(ClIocNodeAddressT node);

typedef struct
{
    ClPlugin pluginInfo;
    clAmfPluginNotificationCallbackT clAmfNotificationCallback;
    clAmfPluginNotificationRunT clAmfRunTask;
    clAmfPluginNodeStatusT clAmfNodeStatus;
} ClAmfPluginApi;


/*
 * Global AMF plugin variables
 */
ClPluginHandle* clAmfPluginHandle = NULL;
ClAmfPluginApi* clAmfPlugin = NULL;

#ifdef __cplusplus
}
#endif
#endif
