#ifndef CLCMPLUGINAPI_H
#define CLCMPLUGINAPI_H

#include <clPluginApi.h>
#include <clDebugApi.h>
#include <SaHpi.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CL_CM_PLUGIN_VERSION 1
#define CL_CM_PLUGIN_ID 0x58429450
#define CL_CM_PLUGIN_NAME "cmPolicy.so"


typedef struct
{
    ClPlugin pluginInfo;
    ClRcT (*clHpiEventFilter)(SaHpiSessionIdT session, SaHpiEventT* event, SaHpiRdrT* rdr, SaHpiRptEntryT* rpt);
} ClCmPluginApi;

#ifdef __cplusplus
}
#endif   
#endif
