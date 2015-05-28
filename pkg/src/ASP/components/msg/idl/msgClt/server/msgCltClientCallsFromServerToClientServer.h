
/*********************************************************************
* File: 
*********************************************************************/
/*********************************************************************
* Description : This file contains the declartions for server stub
*               routines
*     
* THIS FILE IS AUTO-GENERATED BY OPENCLOVIS IDE. EDIT THIS FILE AT
* YOUR OWN RISK. ANY CHANGE TO THIS FILE WILL BE OVERWRITTEN ON
* RE-GENERATION.
*     
*********************************************************************/

#ifndef _MSGCLTCLIENTCALLSFROMSERVERTOCLIENT_SERVER_H_
#define _MSGCLTCLIENTCALLSFROMSERVERTOCLIENT_SERVER_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <clXdrApi.h>
#include <clIdlApi.h>
#include <ipi/clRmdIpi.h>
#include "../clientIDDefinitions.h"
#include "xdrSaMsgQueueGroupNotificationBufferT.h"



ClRcT clMsgClientsTrackCallback_4_0_0(CL_IN ClHandleT  clientHandle, CL_IN ClNameT*  pGroupName, CL_IN SaMsgQueueGroupNotificationBufferT_4_0_0*  pNotification);

ClRcT clMsgClientsTrackCallbackResponseSend_4_0_0(CL_IN ClIdlHandleT idlHdl,CL_IN ClRcT retCode);

ClRcT clMsgClientsMessageReceiveCallback_4_0_0(CL_IN ClHandleT  clientHandle, CL_IN ClHandleT  qHandle);

ClRcT clMsgClientsMessageReceiveCallbackResponseSend_4_0_0(CL_IN ClIdlHandleT idlHdl,CL_IN ClRcT retCode);


#ifdef __cplusplus
}
#endif
#endif /*_MSGCLTCLIENTCALLSFROMSERVERTOCLIENT_SERVER_H_*/