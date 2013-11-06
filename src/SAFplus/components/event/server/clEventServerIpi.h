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
 * File        : clEventServerIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This header contains the server specific info.
 *
 *
 *****************************************************************************/

#ifndef _EVT_CORE_H_
# define _EVT_CORE_H_

# ifdef __cplusplus
extern "C"
{
# endif

# include "clCommon.h"
# include "clRuleApi.h"
# include "clIocApi.h"
# include "clEoApi.h"
# include <clCpmApi.h>
# include "clEventApi.h"
# include "clEventCommonIpi.h"  /* To avoid dependencies */

    /*
     * @ 
     */
# ifdef RETENTION_ENABLE
#  undef RETENTION_ENABLE
# endif

    typedef struct EvtHead
    {
        ClEoExecutionObjT *evtEOId; /* Reference to the EM's EO */
        ClEoStateT state;       /* State of the EM */
        ClUint32T emUpTime;
    } ClEvtHeadT;


    extern ClEvtHeadT gEvtHead;
    extern ClUint32T gEvtInitDone;  /* This holds EM is initialized or not */
    extern ClUint32T gEvtCurrentState;  /* This holds state of the current EM
                                         * instance state */

    /*
     * The following API are implemented in clEventMain.c to hide the database
     * handle 
     */
    extern ClRcT clEvtHandleDatabaseInit(void);
    extern ClRcT clEvtHandleDatabaseExit(void);


    /*
     * The following API has been implemented in rule.c exclusively for event 
     */
    extern ClRcT clRuleExprPrintToMessageBuffer(ClDebugPrintHandleT dbgHandle,
                                                ClRuleExprT *expr);

/**************************************************************************************
                                EM Internal Defenations
**************************************************************************************/

    /*
     * Sate of the EM instance slave/master-primary/master-slave 
     */

# define CL_EVT_ADD_USER_AND_CHANNEL 1
# define CL_EVT_ADD_USER             2

# define CL_EVT_CKPT_REQUEST     0x1
# define CL_EVT_NORMAL_REQUEST   0x2

# define CL_EVT_EVENT_ID_GENERATE(eventId) \
do \
{ \
            (eventId) = clIocLocalAddressGet(); \
            (eventId) = ((eventId) << 32) | 0xFFFFFFFF; \
} while(0)


/**************************************************************************************
                              Log Related Macros
**************************************************************************************/

#define CL_EVENT_LOG_AREA_SRV "SRV"

/**************************************************************************************
                                EM Channel Related structure
**************************************************************************************/
    typedef struct EvtUserOfECHInfo
    {
        ClUint32T userOfTheChannel;
        ClUint16T evtChannelID;
        ClUint8T evtChannelUserType;
    } ClEvtUserOfECHInfoT;

    typedef struct EvtChannelInfo
    {
        /*
         * Head for global local event channels 
         */
        ClCntHandleT evtChannelContainer;   /* Container keeps the information
                                             * about channel */
        ClCntHandleT userOfECHData; /* This is used to store ECH opened users
                                     * information */
    } ClEvtChannelInfoT;

/** Event Channel Information which contains masking, EventType and retention Event
    Informations.
**/
    typedef struct EvtChannelDB
    {
        ClOsalMutexIdT channelLevelMutex;   /* Mutex will protect channelId
                                             * level */
        ClCntHandleT evtChannelUserInfo;    /* List of users who are opened
                                             * event channel */
        ClCntHandleT eventTypeInfo; /* This is used to store the Event Type
                                     * Information */
# ifdef RETENTION_ENABLE
        ClCntHandleT retentionCntHdl;   /* This is used to store Retention *
                                         * event information */
        ClOsalMutexIdT retentionMutex;
# endif
        ClBufferHandleT subsCkptMsgHdl;  /* To hold the msg buffer for
                                                 * Check Pointing Subscriber
                                                 * Info */
        ClUint16T subscriberRefCount;   /* No of users opened this channel for
                                         * subscription purpose */
        ClUint16T publisherRefCount;    /* No of users opened this channel for
                                         * publishing purpose */
        ClUint8T channelScope;  /* channel scope */
    } ClEvtChannelDBT;

/**************************************************************************************
                                EM Subscription Related structures
**************************************************************************************/

/** Event subscription handle. Which is combination of both event channel handle & event subscription handle */
    typedef struct EvtSubscriptionHandle
    {
        ClUint32T functionID;   /* Function ID which is register for given
                                 * event type */
        ClEvtUserIdT userId;    /* User ID */
        ClUint16T evtType;      /* Event type */

    } ClEvtSubscriptionHandleT;


/** Event subscription information. From this EM will identify the 
    subscriber information for given event & event channel 
**/
    typedef struct EvtSubscribeInfo
    {
        ClIocPortT subscriberAddress;   /* Comm-port Address of a subscriber */
        ClUint32T userOfEventChannel;   /* User info opened the channel */
    } ClEvtSubscribeInfoT;

    typedef struct EvtSubsKey
    {
        ClEventSubscriptionIdT subscriptionId;   /* Subscription ID */
        ClEvtUserIdT userId;
        ClUint64T pCookie;      /* This field is not really used for key this
                                 * is just used to store the user cookie info */
        ClUint32T externalAddress;
    } ClEvtSubsKeyT;

# define CL_EVT_TYPE_RBE_KEY_TYPE 	0x1
# define CL_EVT_TYPE_DATA_KEY_TYPE 	0x2

    typedef union EvtEventTypeRbeAndData
    {
        ClRuleExprT *pRbeExpr;
        ClUint8T *pData;
    } EvtEventTypeRbeAndDataT;

    typedef struct EvtEventTypeKey
    {
        ClUint32T flag;         /* To decide the type of compare */
        EvtEventTypeRbeAndDataT key;
        ClUint32T dataLen;
    } ClEvtEventTypeKeyT;


/**************************************************************************************
                                Event Publish Related structures
**************************************************************************************/
    typedef struct EvtPublishInfo
    {
        ClEvtEventTypeKeyT *pEventTypeKey;
        ClEvtEventPrimaryHeaderT *pEvtPrimaryHeader;

    } ClEvtPublishInfoT;



#ifdef NTR
/**************************************************************************************
                                EM Event suppression Related structure
**************************************************************************************/
    typedef struct EvtSuppressionInfo
    {
        ClUint32T suppFlag;
        SaNameT publisherName;
        ClRuleExprT *filterRule;
        ClRuleExprT *payloadRule;
    } ClEvtSuppressionInfoT;
#endif
/**************************************************************************************
                                EM Event type information
**************************************************************************************/
    typedef struct EvtEventTypeInfo
    {
        ClUint16T refCount;     /* No Of subscriber for this Event type */
        ClCntHandleT subscriberInfo;    /* Inforamation about subscriber */
    } ClEvtEventTypeInfoT;


/**************************************************************************************
                                EM Global Variables
**************************************************************************************/
    /*
     * Primary/Secondary Master IOC Address 
     */
    extern ClIocPhysicalAddressT gEvtPrimaryMasterAddr;    /* Primary master IOC
                                                     * address */
    extern ClIocPhysicalAddressT gEvtSecondaryMasterAddr;  /* Secondary master IOC
                                                     * address */

    /*
     * Global/Local Channel Information stored in this container 
     */
    extern ClEvtChannelInfoT *gpEvtGlobalECHDb;
    extern ClEvtChannelInfoT *gpEvtLocalECHDb;

    /*
     * Mutexs to protect ECH/G & ECH/L 
     */
    extern ClOsalMutexIdT gEvtGlobalECHMutex;
    extern ClOsalMutexIdT gEvtLocalECHMutex;

    /*
     * Primary/Secondary Master information 
     */
    extern ClCntHandleT gEvtMasterECHHandle;
    /*
     * Mutexs to protect Master Information 
     */
    extern ClOsalMutexIdT gEvtMasterECHMutex;


/****************************************************************************************
                        Local Functions
****************************************************************************************/
    ClRcT VDECL(clEvtInitializeLocal)(ClEoDataT cData,
                               ClBufferHandleT inMsgHandle,
                               ClBufferHandleT outMsgHandle);
    ClRcT clEvtFinalizeLocal(ClEoDataT cData,
                             ClBufferHandleT inMsgHandle,
                             ClBufferHandleT outMsgHandle);

    ClRcT VDECL(clEvtChannelOpenLocal)(ClEoDataT cData,
                                ClBufferHandleT inMsgHandle,
                                ClBufferHandleT outMsgHandle);
    ClRcT clEvtChannelCloseLocal(ClEoDataT cData,
                                 ClBufferHandleT inMsgHandle,
                                 ClBufferHandleT outMsgHandle);
    ClRcT VDECL(clEvtEventSubscribeLocal)(ClEoDataT cData,
                                   ClBufferHandleT inMsgHandle,
                                   ClBufferHandleT outMsgHandle);

    ClRcT VDECL(clEvtEventUnsubscribeAllLocal)(ClEoDataT cData,
                                        ClBufferHandleT inMsgHandle,
                                        ClBufferHandleT outMsgHandle);

    ClRcT clContNodeAddAndNodeGet(ClCntHandleT containerHandle,
                                  ClCntKeyHandleT userKey,
                                  ClCntDataHandleT userData, ClRuleExprT *pExp,
                                  ClCntNodeHandleT *pNodeHandle);
    ClRcT VDECL(clEvtEventPublishLocal)(ClEoDataT cData,
                                 ClBufferHandleT inMsgHandle,
                                 ClBufferHandleT outMsgHandle);
    ClRcT VDECL(clEvtEventPublishExternal)(ClEoDataT cData,
                                 ClBufferHandleT inMsgHandle,
                                 ClBufferHandleT outMsgHandle);

    ClRcT clEvtSuppressionSetLocal(ClEoDataT cData,
                                   ClBufferHandleT inMsgHandle,
                                   ClBufferHandleT outMsgHandle);
    ClRcT clEvtSuppressionClearLocal(ClEoDataT cData,
                                     ClBufferHandleT inMsgHandle,
                                     ClBufferHandleT outMsgHandle);

    void clEvtChannelInfoGet(ClEventChannelHandleT evtChannelHandle,
                             ClOsalMutexIdT *pMutexId, ClUint16T *pEvtChannelID,
                             ClUint8T *pEvtChannelScope,
                             ClUint8T *pEvtChannelUserType,
                             ClEvtChannelInfoT **ppEvtChannelInfo);
    ClRcT VDECL(clEvtEventPublishProxy)(ClEoDataT cData,
                                 ClBufferHandleT inMsgHandle,
                                 ClBufferHandleT outMsgHandle);

    ClRcT VDECL(clEvtNackSend)(ClEoDataT cData, ClBufferHandleT inMsgHandle,
                        ClBufferHandleT outMsgHandle);

    ClRcT clEvtChannelDBInit();
    ClRcT clEvtChannelDBClean();

    ClRcT clEventDebugRegister(ClEoExecutionObjT *pEoObj);
    ClRcT clEventDebugDeregister(ClEoExecutionObjT *pEoObj);
    ClRcT clEvtChannelDBDelete(ClEvtChannelDBT *pEvtChannel);
# ifdef __cplusplus
}
# endif

#endif                          /* _EVT_CORE_H_ */
