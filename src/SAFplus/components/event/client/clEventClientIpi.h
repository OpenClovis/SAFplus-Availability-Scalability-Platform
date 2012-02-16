/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * For more  information, see  the file  COPYING provided with this
 * material.
 */
/*******************************************************************************
 * ModuleName  : event                                                         
 * File        : clEventClientIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *          This header file contains the information internal to the
 *          client.
 *
 *
 *****************************************************************************/

#ifndef _EVT_CLIENT_H_
# define _EVT_CLIENT_H_

# ifdef __cplusplus
extern "C"
{
# endif

# include "clEventApi.h"
# include "clQueueApi.h"
# include "clEventCommonIpi.h"
# include "clHandleApi.h"
/**************************************************************************************
                                Definition
**************************************************************************************/
# define CL_EVT_INIT_VALIDATE(eoObj,rc,pEvtClientHead)\
do{\
    rc = clEoMyEoObjectGet(&eoObj);\
    if(CL_OK != rc)\
    {\
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get EO object [0x%X]",rc));\
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME, CL_EVENT_LOG_MSG_1_INTERNAL_ERROR, rc);\
        CL_FUNC_EXIT();\
        return CL_EVENT_INTERNAL_ERROR;\
    }\
    rc = clEoPrivateDataGet (eoObj, CL_EO_EVT_EVENT_DELIVERY_COOKIE_ID, (void**)&pEvtClientHead);\
    if((CL_OK != rc) && (CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(rc)))\
    {\
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get EO private data [0x%X]",rc));\
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME, CL_EVENT_LOG_MSG_1_INTERNAL_ERROR, rc);\
        CL_FUNC_EXIT();\
        return CL_EVENT_INTERNAL_ERROR;\
    }\
    else if(CL_ERR_NOT_EXIST == rc)\
    {\
        CL_FUNC_EXIT();\
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME, CL_LOG_MESSAGE_0_COMPONENT_UNINITIALIZED);\
        return CL_EVT_INIT_NOT_DONE;\
    }\
}while(0);


# define CL_EVT_CHANNEL_NAME_VALIDATE(pName)\
do{\
    if(NULL == (pName))\
    {\
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Passed NULL for channel name\n\r"));\
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME, CL_EVENT_LOG_MSG_1_NULL_ARGUMENT, "Channel Name");\
        CL_FUNC_EXIT();\
        return CL_EVENT_ERR_NULL_PTR;\
    }\
    if(0 == (pName)->length)\
    {\
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Channel Name has 0 lenth\n\r"));\
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME, CL_EVENT_LOG_MSG_1_INVALID_PARAMETER, "Channel Name has 0 length");\
        CL_FUNC_EXIT();\
        return CL_EVENT_ERR_INVALID_PARAM;\
    }\
    if((pName)->length >= CL_MAX_NAME_LENGTH)\
    {\
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Channel Name too long\n\r"));\
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME, CL_EVENT_LOG_MSG_1_INVALID_PARAMETER, "Channel Name too long");\
        CL_FUNC_EXIT();\
        return CL_EVENT_ERR_INVALID_PARAM;\
    }\
}while(0);

# define CL_EVT_CHANNEL_KEY_GEN(channelScope,channelId) ((((channelScope)|0xFFFC)<<16)|(channelId))

# define CL_EVT_CHANNEL_SCOPE_GET_FROM_FLAG(flag) (flag&0x1)
# define CL_CHANNEL_HANDLE 1
# define CL_EVENT_HANDLE 2
# define CL_EVENT_INIT_HANDLE 3

/**************************************************************************************
                              Log Related Macros
**************************************************************************************/

#define CL_EVENT_LOG_AREA_CLT "CLT"

    /*
     * This contain callback information and channel related information, which 
     * are associated with given EO. 
     */
    typedef struct EvtClientHead
    {
        ClHandleDatabaseHandleT evtClientHandleDatabase;
   
    } ClEvtClientHeadT;
    
    typedef struct EvtHdlDbData
    {
        ClUint8T handleType;

    } ClEvtHdlDbDataT;
     
    typedef struct EvtInitInfo
    {
        ClUint8T handleType;
        ClUint8T queueFlag;
        ClEventVersionCallbacksT *pEvtCallbackTable;
        ClUint32T numCallbacks;
        ClInt32T writeFd;
        ClInt32T readFd;
        ClQueueT cbQueue;
        ClOsalMutexIdT cbMutex;
        ClHandleT servHdl; 
        ClInt32T  numReceiveWaiters;
        ClOsalCondIdT receiveCond;
        ClBoolT   receiveStatus;
    } ClEvtInitInfoT;

    /*
     * Selection Object related changes 
     */

    typedef enum EvtCallbackId
    {
        CL_EVT_PUBLISH_CALLBACK,    /* 0 */
        CL_EVT_CHANNEL_CALLBACK,    /* 1 */

    } ClEvtCallbackIdT;

    typedef struct EvtCbQueueData
    {
        ClEvtCallbackIdT cbId;
        void *cbArg;

    } ClEvtCbQueueDataT;


    typedef struct EvtEventPublishInfo
    {
        ClUint8T version;
        ClEventSubscriptionIdT subscriptionId;
        ClEventHandleT eventHandle;
        ClSizeT eventDataSize;
    } ClEvtEventPublishInfoT;

    typedef struct EvtClientChannelInfo
    {
        ClUint8T handleType;
        ClEventInitHandleT evtHandle;   /* The handle created in
                                         * clEventInitialize */
        ClUint32T evtChannelKey;
        ClNameT evtChannelName;
        ClUint8T flag;

    } ClEvtClientChannelInfoT;

    typedef struct EvtClientAsyncChanOpenCbArg
    {
        ClEventInitHandleT evtHandle;
        ClRcT apiResult;
        ClInvocationT invocation;
        ClEventChannelHandleT channelHandle;

    } ClEvtClientAsyncChanOpenCbArgT;


/**************************************************************************************
                                Function Prototype
**************************************************************************************/
    ClRcT clEvtSubsInfoShow(ClNameT *pChannelName, ClUint8T channelScope,
                            ClUint8T disDetailFlag,
                            ClDebugPrintHandleT dbgPrintHandle);

    ClRcT clEvtClientChannelInfoGet(ClCntNodeHandleT nodeHandle,
                                    ClEvtClientHeadT *pEvtClientHead,
                                    ClEvtClientChannelInfoT **ppEvtChannelInfo);

    
    ClRcT VDECL(clEvtEventReceive)(ClEoDataT data, ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle);
	
	ClRcT	clEventLibTableInit(void);
/*************************************************************/

# ifdef __cplusplus
}
# endif

#endif                          /* _EVT_CLIENT_H_ */
