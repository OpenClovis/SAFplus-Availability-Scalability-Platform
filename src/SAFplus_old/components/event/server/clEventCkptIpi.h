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
 * File        : clEventCkptIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *          This header file contains the CKPT related functionality EM
 *          employs.
 *
 *
 *****************************************************************************/

#ifndef _EVT_CKPT_SERVER_H_
# define _EVT_CKPT_SERVER_H_

# ifdef __cplusplus
extern "C"
{
# endif

# include "clCommon.h"
# include "clRuleApi.h"
# include "clEoApi.h"
# include "clCkptApi.h"
# include "clCkptExtApi.h"
# include "clEventApi.h"
# include "clEventServerIpi.h"
# include "clEventUtilsIpi.h"
# include "clHandleApi.h"

    /*
     * To Enable Check Pointing Support 
     */
    /*
     * @Log
     */
# if 1
#  define CKPT_ENABLED
# endif

    /*
     * To Enable Logging CKPT Data into a flat file 
     * Create the CKPT Data file for debugging if CKPT 
     * not working.
     */
# if 0
#  define FLAT_FILE
# endif


# define CL_EVT_CKPT_COUNT           50 /* No. of Check Points intended - CKPT
                                         * Names */
# define CL_EVT_CKPT_NAME            "Cl_Event_Check_Point"
# define CL_EVT_CKPT_DATASET_COUNT   2
# define CL_EVT_CKPT_ORDER           0x0

# define CL_EVT_CKPT_INCREMENTAL     0X1
# define CL_EVT_CKPT_COMPLETE    	0x0

# define CL_EVT_CKPT_GROUP_ID        0x0

# define CL_EVT_CKPT_OP_INITIALIZE   0x1
# define CL_EVT_CKPT_OP_FINALIZE     0x0

# define CL_EVT_CKPT_OP_ECH_OPEN    	0x1
# define CL_EVT_CKPT_OP_ECH_CLOSE   	0x0


# define CL_EVT_CKPT_OP_SUBSCRIBE    0x1
# define CL_EVT_CKPT_OP_UNSUBSCRIBE  0x0


# define CL_EVT_CKPT_USER_DSID       0x1    /* Only one Dataset needed */
    /*
     ** Dynamic Dataset ID based on the scope of the
     ** channel. Adding 1 arbitrarily to avoid 0 as
     ** Dataset ID
     */
#define CL_EVT_CKPT_ECH_DSID_BASE  (100)
#define CL_EVT_CKPT_SUBS_DSID_BASE (200)
# define CL_EVT_CKPT_ECH_DSID(scope) (CL_EVT_CKPT_ECH_DSID_BASE + (scope)+2)    /* Since 1 is for user */
# define CL_EVT_CKPT_SUBS_DSID(scope) (CL_EVT_CKPT_SUBS_DSID_BASE + (scope)+1)

    /*
     ** Toggle the scope of the channel
     */
# define CL_EVT_CKPT_TOGGLE_SCOPE(scope) (((scope)== CL_EVENT_LOCAL_CHANNEL) ? \
                                 CL_EVENT_GLOBAL_CHANNEL : CL_EVENT_LOCAL_CHANNEL)

    /*
     ** Threshold for the counter calls
     */
# define CL_EVT_CKPT_THRESHOLD  0x5 /* This needs to dynamic */

    /*
     ** TypeDefs for User Info Check Pointing 
     */
    typedef struct clEvtCkptUserInfo
    {
        ClUint32T operation;    /* Init / Finalize */
        ClEvtUserIdT userId;    /* User ID */
        ClUint8T isExternal;

    } ClEvtCkptUserInfoT;

    typedef struct clEvtCkptUserInfoWithLen
    {
        ClEvtCkptUserInfoT *pUserInfo;
        ClUint32T userInfoLen;
        ClUint32T finalizeCount;

    } ClEvtCkptUserInfoWithLenT;


    typedef struct clEvtCkptUserInfoWithMutex
    {
        ClUint16T type;         /* Inc / Norm check point */
        ClUint16T operation;    /* Init / Finalize */
        ClHandleDatabaseHandleT databaseHandle;/* User Info HandleDatabase */
        ClEvtInitRequestT *pInitReq;    /* Initialize Request */

    } ClEvtCkptUserInfoWithMutexT;


    /*
     * TypeDefs for ECH Check Pointing 
     */

    typedef struct clEvtCkptECHInfo
    {
        ClUint32T operation;    /* Open/Close */
        ClEvtUserIdT userId;    /* User Id */
        ClUint32T chanHandle;   /* Channel Handle */
        SaNameT chanName;       /* Channel Name */
        ClUint8T isExternal;
    } ClEvtCkptECHInfoT;

    typedef struct clEvtCkptECHInfoWithLen
    {
        ClEvtCkptECHInfoT *pECHInfo;
        ClUint32T echInfoLen;
        ClUint32T echCloseCount;
        ClUint32T scope;

    } ClEvtCkptECHInfoWithLenT;

    typedef struct clEvtCkptECHInfoWithMutex
    {
        ClUint8T type;          /* Inc / Norm check point */
        ClUint8T scope;         /* Global/Local */
        ClUint16T operation;    /* Open/Close */
        ClOsalMutexIdT mutexId; /* Mutex Id */

        ClCntHandleT channelContainer;  /* Container Handle to User Info */

        ClEvtChannelOpenRequestT *pOpenReq;
        ClEvtUnsubscribeEventRequestT *pUnsubsReq;

    } ClEvtCkptECHInfoWithMutexT;


    /*
     * TypeDefs for Subs Check Pointing 
     */

    typedef struct clEvtCkptSubsInfo
    {
        ClUint32T operation;    /* Subscribe/Unsubscribe */
        ClEvtUserIdT userId;    /* User Id */
        ClUint32T chanHandle;   /* Channel Handle */
        ClUint64T cookie;       /* Cookie */
        ClEventSubscriptionIdT subsId;  /* Subscription Id */
        ClIocPortT commPort;    /* Port */
        ClUint8T *packedRbe;
        ClUint32T packedRbeLen;
        ClUint32T externalAddress;
    } ClEvtCkptSubsInfoT;

    typedef struct clEvtCkptSubsInfoWithLen
    {
        ClEvtCkptSubsInfoT *pSubsInfo;
        ClUint32T subsInfoLen;
        ClUint32T scope;
        SaNameT chanName;

    } ClEvtCkptSubsInfoWithLenT;


    typedef struct clEvtCkptSubsInfoWithMsgHdl
    {
        ClEvtCkptSubsInfoT *pSubsInfo;
        ClBufferHandleT msgHdl;

    } ClEvtCkptSubsInfoWithMsgHdlT;

    typedef struct clEvtCkptSubsInfoWithMutex
    {
        ClUint16T type;         /* Inc / Norm check point */
        ClUint16T operation;    /* Subscribe/Unsubscribe */
        ClOsalMutexIdT mutexId; /* Mutex Id */
        ClUint32T scope;        /* Channel Scope */

        /*
         * For easy access 
         */
        ClUint32T chanHandle;
        SaNameT *pChanName;

        ClCntHandleT eventTypeContainer;    /* Event Type Container Handle */

        ClEvtSubscribeEventRequestT *pSubsReq;
        ClEvtUnsubscribeEventRequestT *pUnsubsReq;

        /*
         ** CKPT Msg buffer Handle for this channel.
         ** This is obtained from subsCkptMsgHdl field of ChannelDB.
         */
        ClBufferHandleT msgHdl;

    } ClEvtCkptSubsInfoWithMutexT;


    extern ClRcT clEvtCkptInit(void);
    extern ClRcT clEvtCkptExit(void);

    extern ClRcT clEvtCkptSubsDSCreate(SaNameT *pChannelName,
                                       ClUint32T channelScope);
    extern ClRcT clEvtCkptSubsDSDelete(SaNameT *pChannelName,
                                       ClUint32T channelScope);

    extern ClRcT clEvtCkptCheckPointAll(void);
    extern ClRcT clEvtCkptUserInfoCheckPoint(void);
    extern ClRcT clEvtCkptECHInfoCheckPoint(void);
    extern ClRcT clEvtCkptSubsInfoCheckPoint(void);
    extern ClRcT clEvtCkptReconstruct(void);

    extern ClRcT clEvtCkptCheckPointInitialize(ClEvtInitRequestT *pEvtInitReq);
    extern ClRcT clEvtCkptCheckPointFinalize(ClEvtUnsubscribeEventRequestT
                                             *pEvtUnsubsReq);

    extern ClRcT clEvtCkptCheckPointChannelOpen(ClEvtChannelOpenRequestT
                                                *pEvtChannelOpenRequest);
    extern ClRcT clEvtCkptCheckPointChannelClose(ClEvtUnsubscribeEventRequestT
                                                 *pEvtUnsubsReq);

    extern ClRcT clEvtCkptCheckPointSubscribe(ClEvtSubscribeEventRequestT
                                              *pEvtSubsReq,
                                              ClEvtChannelDBT *pEvtChannelDB);
    extern ClRcT clEvtCkptCheckPointUnsubscribe(ClEvtUnsubscribeEventRequestT
                                                *pEvtUnsubsReq,
                                                ClEvtChannelDBT *pEvtChannelDB);

    extern ClRcT clEvtCkptUserInfoRead(ClEvtCkptUserInfoWithLenT
                                       *pUserInfoWithLen);
    extern ClRcT clEvtCkptECHInfoRead(ClEvtCkptECHInfoWithLenT
                                      *pECHInfoWithLen);
    extern ClRcT clEvtCkptSubsInfoRead(ClEvtCkptSubsInfoWithLenT
                                       *pSubsInfoWithLen);
    /*
     ** Local Functions for global checkpoint
     */
    extern ClRcT clEventGlobalCkptInitial();
    extern ClRcT clEvtCkptGlobalCheckPointChannelOpen(ClEvtChannelOpenRequestT	*pEvtChannelOpenRequest);
    extern ClRcT clEvtCkptGlobalCheckPointUserInfo(ClEvtInitRequestT *pEvtInitReq);
    extern ClRcT clEvtCkptGlobalCheckPointChannelSub(ClEvtSubscribeEventRequestT *pEvtSubsReq);
    extern ClRcT clEvtUserInfoCkptDelete(ClEvtUnsubscribeEventRequestT  *pEvtUnsubsReq);
    extern ClRcT clEvtChannelSubCkptDelete(ClEvtUnsubscribeEventRequestT  *pEvtUnsubsReq);
    extern ClRcT clEvtChannelOpenCkptDelete(ClEvtUnsubscribeEventRequestT  *pEvtUnsubsReq);
    extern ClRcT clEventGlobalCkptFinalize();
    extern ClRcT clEventExternalRecoverState(ClBoolT switchover);
    extern ClRcT clEventGmsInit(void);



# ifdef __cplusplus
}
# endif

#endif                          /* _EVT_CKPT_SERVER_H_ */
