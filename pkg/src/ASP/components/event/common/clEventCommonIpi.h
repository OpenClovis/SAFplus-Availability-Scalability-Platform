/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : event                                                         
 * File        : clEventCommonIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *          This header contains the info common to the various EM modules.
 *
 *
 *****************************************************************************/

#ifndef _EVT_COMMON_H_
# define _EVT_COMMON_H_

# ifdef __cplusplus
extern "C"
{
# endif


# include <string.h>
# include "clCommon.h"
# include "clDebugApi.h"
# include "clRuleApi.h"
# include "clIocApi.h"

# include "clRmdApi.h"

# include "clEoApi.h"
# include "clEventApi.h"
# include "clEventLog.h"

    /*
     * Added for Versioning 
     */
# define VERSION_READY  
# ifdef VERSION_READY
# endif

    /*
     * EM Timeout for value for making RMD call from EM/L to EM/S 
     */
# define CL_EVT_RMD_TIME_OUT     10*1000 /* 10 second */
# define CL_EVT_RMD_MAX_RETRIES  3 /* No of retried to make RMD call between
                                     * EM/C to EM/S */

    /*
     * CL_TRUE/CL_FALSE value 
     */
# define CL_EVT_FALSE 0
# define CL_EVT_TRUE  1

# define CL_EVT_UNSUBSCRIBE      0x1
# define CL_EVT_CHANNEL_CLOSE    0x2
# define CL_EVT_FINALIZE         0x4


    /*
     * NACK IDs
     */
# define CL_EVT_NACK_PUBLISH     0x1

    typedef struct ClEvtNackInfo
    {
        ClUint8T releaseCode;
        ClUint8T majorVersion;
        ClUint8T minorVersion;
        ClUint8T nackType;
    }
    ClEvtNackInfoT;

    /*
     * Representing RMD functions exposed by EM using proper macros 
     */
# define EO_CL_EVT_NACK_SEND            CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,0x1)
# define EO_CL_EVT_INTIALIZE            CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,0x2)
# define EO_CL_EVT_CHANNEL_OPEN         CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,0x3)
# define EO_CL_EVT_SUBSCRIBE            CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,0x4)
# define EO_CL_EVT_UNSUBSCRIBE          CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,0x5)
# define EO_CL_EVT_PUBLISH              CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,0x6)
# define EO_CL_EVT_PUBLISH_PROXY        CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,0x7)
# ifdef RETENTION_ENABLE
#  define EO_CL_EVT_RETENTION_QUERY     CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,0x8)
# endif
# define EVT_FUNC_ID(n)                 CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,n)

    /* Macro used by event server to make RMD to an event client */
# define CL_EVT_EVENT_DELIVERY_FN_ID     CL_EO_GET_FULL_FN_NUM(CL_EO_EVT_CLIENT_TABLE_ID,1)

    /*
     * MAX Number of bucket for hash tabel 
     */
# define CL_EVT_MAX_ECH_BUCKET           20
# define CL_EVT_MAX_EVENT_TYPE_BUCKET    10

    /*
     * EM Well known comm port which is listening on 
     */
# define CL_EVT_COMM_PORT CL_IOC_EVENT_PORT

/**************************************************************************************
                                EM Utility Macro
**************************************************************************************/
    /*
     * Validating channel open flags 
     */

# define CL_EVT_FLAG_SPACE16 0xFFF8
# define CL_EVT_FLAG_SPACE32 0xFFF80000
# define CL_EVT_VALIDATE_OPEN_CHANNEL_FLAG(X)        ((X)<=0xf &&\
    ((X&CL_EVENT_CHANNEL_PUBLISHER) || (X&CL_EVENT_CHANNEL_SUBSCRIBER)))
# define CL_EVT_VALIDATE_OPEN_CHANNEL_HANDLE(X)      ((((X)&(CL_EVT_FLAG_SPACE32)) == (CL_EVT_FLAG_SPACE32)))
# define CL_EVT_VALIDATE_SUBSCRIBE_HANDLE(X)         (NULL!=((void*)(X)))

    /*
     * Get the ECH-ID & scope & Type of user of the event channel 
     */
# define CL_EVT_CHANNEL_ID_GET(evtChannelHandle,evtChannelId) (evtChannelId = (evtChannelHandle&0xFFFF))

# define CL_EVT_CHANNEL_SCOPE_GET(evtChannelHandle,evtChannelScope)\
((evtChannelScope) = ((evtChannelHandle)>>16)&0x1)

# define CL_EVT_CHANNEL_USER_TYPE_GET(evtChannelHandle,evtChannelUserType)\
((evtChannelUserType) = ((evtChannelHandle)>>16)&0x6)

#ifdef NTR
// #define clLogWrite clEvtLog
// #define clEventLog(_severity, _area, _context, ...)
#endif

# define CL_EVT_SUBSCRIBE_FLAG_VALIDATE(X)\
do{\
    if((X & CL_EVENT_CHANNEL_SUBSCRIBER) == 0)\
    {\
        clLog(CL_LOG_ERROR, "CLT", "VAL", CL_EVENT_LOG_MSG_1_BAD_FLAGS, X);\
        rc = CL_EVENT_ERR_NOT_OPENED_FOR_SUBSCRIPTION;\
        goto failure;\
    }\
}while(0)


# define CL_EVT_PUBLISH_FLAG_VALIDATE(X)\
do{\
    if((X & CL_EVENT_CHANNEL_PUBLISHER) == 0)\
    {\
        clLog(CL_LOG_ERROR, "CLT", "VAL","This channel is not opened it for Publish event");\
        clLog(CL_LOG_ERROR, "CLT", "VAL", CL_EVENT_LOG_MSG_1_BAD_FLAGS, X);\
        rc = CL_EVENT_ERR_NOT_OPENED_FOR_PUBLISH;\
        goto failure;\
    }\
}while(0)

# define CL_EVT_SET_GLOBAL_SCOPE(X) (X=(X|(1<<16)))
# define CL_EVT_SET_LOCAL_SCOPE(X)  (X=((X&(~(1<<16)))))

    /*
     * Form channel handle from ID and flag 
     */
# define CL_EVT_CHANNEL_HANDLE_FORM(flag,evtChannelId) (((flag|CL_EVT_FLAG_SPACE16)<<16)|(evtChannelId))
# define CL_EVT_CHANNEL_HANDLE_FROM_KEY_FLAG(flag,evtChannelKey) (((flag|CL_EVT_FLAG_SPACE16)<<16)|(evtChannelKey&0xffff))

    /*
     * Validate Init od EM is done or not 
     */
# define CL_EVT_INIT_DONE_VALIDATION()\
do{\
    if(CL_EVT_FALSE == gEvtInitDone)\
    {\
        clLog(CL_LOG_ERROR, "CLT", "VAL","EM Init is not done\n");\
        return CL_EVENT_ERR_INIT_NOT_DONE;\
    }\
}while(0);


#define CL_EVT_DATA_VERSION_BASE (0x1)

/**************************************************************************************
                                EM Packing Information Related structure
**************************************************************************************/

    typedef struct EvtUserId
    {
        ClEoIdT eoIocPort;   /* The EOid of the EO */
        ClEventInitHandleT evtHandle;   /* The handle generated in
                                         * clEventInitialize */
        ClHandleT clientHandle; /* Handle obtained from the client */
    } ClEvtUserIdT;

    typedef struct EvtInitRequest
    {
        ClUint8T releaseCode;
        ClUint8T majorVersion;
        ClUint8T minorVersion;
        ClUint8T reserved;
        ClEvtUserIdT userId;
        ClHandleT clientHdl;
    } ClEvtInitRequestT;

    typedef struct EvtChannelOpenRequest
    {
        ClUint8T releaseCode;
        ClUint8T majorVersion;
        ClUint8T minorVersion;
        ClUint8T reserved;
        ClEventChannelHandleT evtChannelHandle;
        ClEvtUserIdT userId;
        ClNameT evtChannelName;

    } ClEvtChannelOpenRequestT;

    typedef struct EvtSubscribeEventRequest
    {
        ClUint8T releaseCode;
        ClUint8T majorVersion;
        ClUint8T minorVersion;
        ClUint8T reserved;
        ClEventChannelHandleT evtChannelHandle;
        ClIocPortT subscriberCommPort;
        ClEvtUserIdT userId;
        ClEventSubscriptionIdT subscriptionId;
        ClUint64T pCookie;
        ClNameT evtChannelName;
        ClUint8T *packedRbe;
        ClUint32T packedRbeLen;
    } ClEvtSubscribeEventRequestT;

    typedef struct EvtUnsubscribeEventRequest
    {
        ClUint8T releaseCode;
        ClUint8T majorVersion;
        ClUint8T minorVersion;
        ClUint8T reserved;
        ClEventChannelHandleT evtChannelHandle;
        ClEvtUserIdT userId;
        ClEventSubscriptionIdT subscriptionId;
        ClNameT evtChannelName;
        ClUint8T reqFlag;

    } ClEvtUnsubscribeEventRequestT;


    typedef struct EvtEventPrimaryHeader
    {
        ClUint8T releaseCode;
        ClUint8T majorVersion;
        ClUint8T minorVersion;
        ClUint8T version;

        ClUint8T eventPriority;
        ClUint8T channelScope;
        ClUint16T channelId;

        ClUint16T channelNameLen;
        ClUint16T publisherNameLen;

        ClUint32T patternSectionLen;    /* Length of the Pattern Section */
        ClUint32T noOfPatterns; /* No. of patterns in the Pattern Section */

        /*
         * Introduced to avoid padding by compiler.
         * This can be used in future.
         */
        ClUint32T reservedWord;

        ClUint64T eventId;
        ClUint64T eventDataSize;    /* Added here for easier access */

        ClInt64T retentionTime;
        ClInt64T publishTime;

    } ClEvtEventPrimaryHeaderT;

    typedef struct EvtEventSecondaryHeader
    {
        ClEventInitHandleT evtHandle;   /* The handle generated in
                                         * clEventInitialize */
        ClEventSubscriptionIdT subscriptionId;
        ClUint64T pCookie;

    } ClEvtEventSecondaryHeaderT;

    typedef struct EvtChannelKey
    {
        ClUint16T channelId;
        ClNameT channelName;

    } ClEvtChannelKeyT;

    typedef struct EvtEventHandle
    {
        ClUint8T handleType; 
        ClBufferHandleT msgHandle;
        ClEventChannelHandleT evtChannelHandle;

    } ClEvtEventHandleT;
# ifdef __cplusplus
}
# endif

#endif                          /* _EVT_COMMON_H_ */
