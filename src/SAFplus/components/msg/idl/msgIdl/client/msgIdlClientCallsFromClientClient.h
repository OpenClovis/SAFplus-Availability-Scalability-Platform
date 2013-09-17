
/*********************************************************************
* ModuleName  : idl
*********************************************************************/
/*********************************************************************
* Description : This file contains the declartions for client stub
*               routines
*     
* THIS FILE IS AUTO-GENERATED BY OPENCLOVIS IDE. EDIT THIS FILE AT
* YOUR OWN RISK. ANY CHANGE TO THIS FILE WILL BE OVERWRITTEN ON
* RE-GENERATION.
*     
*********************************************************************/
#ifndef _MSG_IDL_CLIENT_CALLS_FROM_CLIENT_CLIENT_H_
#define _MSG_IDL_CLIENT_CALLS_FROM_CLIENT_CLIENT_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <clXdrApi.h>
#include <clIdlApi.h>
#include "../clientIDDefinitions.h"



ClRcT clMsgInitClientSync_4_0_0(CL_IN ClIdlHandleT handle, CL_IN ClUint32T* pVersion, CL_IN ClHandleT clientHandle, CL_OUT ClHandleT* pMsgHandle);

ClRcT clMsgFinClientSync_4_0_0(CL_IN ClIdlHandleT handle, CL_IN ClHandleT msgHandle);

ClRcT clMsgMessageGetClientSync_4_0_0(CL_IN ClIdlHandleT handle, CL_IN SaNameT* pQueueName, CL_IN ClInt64T timeout);


#ifdef __cplusplus
}
#endif
#endif /*_MSG_IDL_CLIENT_CALLS_FROM_CLIENT_CLIENT_H_*/
