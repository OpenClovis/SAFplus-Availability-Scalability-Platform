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
 * File        : clEvtClientMain.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *          This module contains the library support for the EM API.
 *          It provides an Interface to the EM server.
 *****************************************************************************/

#ifndef _CL_EVT_CLIENT_MAIN_H_
# define _CL_EVT_CLIENTMAIN_H_

# ifdef __cplusplus
extern "C"
{
# endif

ClRcT channelHdlWalk(ClHandleDatabaseHandleT databaseHandle, ClHandleT handle, void * pCookie);

ClRcT clEvtClientInit(ClEoExecutionObjT **ppEoObj, ClEvtClientHeadT **ppEvtClientHead);

ClRcT clEventCpmCleanup(ClCpmEventPayLoadT *pEvtCpmCleanupInfo);

ClRcT clEvtInitValidate(ClEoExecutionObjT **pEoObj, ClEvtClientHeadT **ppEvtClientHead);

ClRcT clEvtQueueCallback(ClEvtInitInfoT *pInitInfo, ClEvtCallbackIdT cbId, void *cbArg);

void handleDestroyCallback(void * pInstance);

void clEvtCallbackDispatcher(ClEvtCbQueueDataT *pQueueData, ClEvtInitInfoT *pInitInfo);

ClRcT clEvtInitHandleValidate(ClEventInitHandleT evtHandle, ClEvtClientHeadT *pEvtClientHead);

ClRcT clEvtChannelOpenEpilogue(ClRcT rc, ClEvtClientHeadT *pEvtClientHead, ClEventChannelHandleT *pEvtChannelHandle, ClBufferHandleT *pInMsgHandle, ClBufferHandleT *pOutMsgHandle);

ClRcT clEvtChannelOpenPrologue(ClEventInitHandleT evtHandle, const SaNameT *pChannelName, ClEventChannelOpenFlagsT evtChannelOpenFlag,
        ClEventChannelHandleT *pEvtChannelHandle, ClEvtClientHeadT **ppEvtClientHead, ClBufferHandleT *pInMsgHandle,
        ClBufferHandleT *pOutMsgHandle);

void clEvtClientInitInfoDelete(ClCntKeyHandleT userKey, ClCntDataHandleT userData);

ClRcT eventFinalizeHandleDBWalk(ClHandleDatabaseHandleT databaseHandle, ClHandleT handle, void *pCookie);

void clEvtAsyncChanOpenCbReceive(ClRcT apiResult, void *pCookie, ClBufferHandleT inMsgHandle, ClBufferHandleT outMsgHandle);

ClInt32T clEvtClientEvtHandleCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2);

ClInt32T clEvtClientECHUserKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2);

void clEvtClientECHUserUserDelete(ClCntKeyHandleT userKey, ClCntDataHandleT userData);

static void evtEventDeliverCallbackDispatch(ClEvtInitInfoT *pInitInfo, ClUint8T version, ClEventSubscriptionIdT subscriptionId,
                                            ClEventHandleT eventHandle, ClSizeT dataSize);


# ifdef __cplusplus
}
# endif


#endif  /*_CL_EVT_CLIENT_MAIN_H_ */

