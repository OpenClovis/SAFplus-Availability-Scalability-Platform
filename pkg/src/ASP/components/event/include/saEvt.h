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
 * File        : saEvt.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

/*
 * Header file of SA Forum AIS EVT APIs (SAI-AIS-B.01.00.09) compiled on
 * 21SEP2004 by sayandeb.saha@motorola.com. 
 */

#ifndef _SA_EVT_H
# define _SA_EVT_H

# include <saAis.h>

# ifdef  __cplusplus
extern "C"
{
# endif

    typedef SaUint64T SaEvtHandleT;
    typedef SaUint64T SaEvtEventHandleT;
    typedef SaUint64T SaEvtChannelHandleT;
    typedef SaUint32T SaEvtSubscriptionIdT;


    typedef void (*SaEvtChannelOpenCallbackT) (SaInvocationT invocation,
                                               SaEvtChannelHandleT
                                               channelHandle,
                                               SaAisErrorT error);
    typedef void (*SaEvtEventDeliverCallbackT) (SaEvtSubscriptionIdT
                                                subscriptionId,
                                                SaEvtEventHandleT eventHandle,
                                                SaSizeT eventDataSize);

    typedef struct
    {
        SaEvtChannelOpenCallbackT saEvtChannelOpenCallback;
        SaEvtEventDeliverCallbackT saEvtEventDeliverCallback;
    } SaEvtCallbacksT;

# define SA_EVT_CHANNEL_PUBLISHER  0x1
# define SA_EVT_CHANNEL_SUBSCRIBER 0x2
# define SA_EVT_CHANNEL_CREATE     0x4
    typedef SaUint8T SaEvtChannelOpenFlagsT;

    typedef struct
    {
        SaSizeT allocatedSize;
        SaSizeT patternSize;
        SaUint8T *pattern;
    } SaEvtEventPatternT;

    typedef struct
    {
        SaSizeT allocatedNumber;
        SaSizeT patternsNumber;
        SaEvtEventPatternT *patterns;
    } SaEvtEventPatternArrayT;


# define SA_EVT_HIGHEST_PRIORITY 0
# define SA_EVT_LOWEST_PRIORITY  3
    typedef SaUint8T SaEvtEventPriorityT;

    typedef SaUint64T SaEvtEventIdT;
# define SA_EVT_EVENTID_NONE 0
# define SA_EVT_EVENTID_LOST 1

# define SA_EVT_LOST_EVENT "SA_EVT_LOST_EVENT_PATTERN"

    typedef enum
    {
        SA_EVT_PREFIX_FILTER = 1,
        SA_EVT_SUFFIX_FILTER = 2,
        SA_EVT_EXACT_FILTER = 3,
        SA_EVT_PASS_ALL_FILTER = 4
    } SaEvtEventFilterTypeT;

    typedef struct
    {
        SaEvtEventFilterTypeT filterType;
        SaEvtEventPatternT filter;
    } SaEvtEventFilterT;

    typedef struct
    {
        SaSizeT filtersNumber;
        SaEvtEventFilterT *filters;
    } SaEvtEventFilterArrayT;

    extern SaAisErrorT saEvtInitialize(SaEvtHandleT * evtHandle,
                                       const SaEvtCallbacksT * callbacks,
                                       SaVersionT *version);
    extern SaAisErrorT saEvtSelectionObjectGet(SaEvtHandleT evtHandle,
                                               SaSelectionObjectT *
                                               selectionObject);
    extern SaAisErrorT saEvtDispatch(SaEvtHandleT evtHandle,
                                     SaDispatchFlagsT dispatchFlags);
    extern SaAisErrorT saEvtFinalize(SaEvtHandleT evtHandle);
    extern SaAisErrorT saEvtChannelOpen(SaEvtHandleT evtHandle,
                                        const SaNameT *channelName,
                                        SaEvtChannelOpenFlagsT channelOpenFlags,
                                        SaTimeT timeout,
                                        SaEvtChannelHandleT *channelHandle);
    extern SaAisErrorT saEvtChannelOpenAsync(SaEvtHandleT evtHandle,
                                             SaInvocationT invocation,
                                             const SaNameT *channelName,
                                             SaEvtChannelOpenFlagsT
                                             channelOpenFlags);
    extern SaAisErrorT saEvtChannelClose(SaEvtChannelHandleT channelHandle);
    extern SaAisErrorT saEvtChannelUnlink(SaEvtHandleT evtHandle,
                                          const SaNameT *channelName);
    extern SaAisErrorT saEvtEventAllocate(SaEvtChannelHandleT channelHandle,
                                          SaEvtEventHandleT *eventHandle);
    extern SaAisErrorT saEvtEventFree(SaEvtEventHandleT eventHandle);
    extern SaAisErrorT saEvtEventAttributesSet(SaEvtEventHandleT eventHandle,
                                               const SaEvtEventPatternArrayT
                                               *patternArray, SaUint8T priority,
                                               SaTimeT retentionTime,
                                               const SaNameT *publisherName);
    extern SaAisErrorT saEvtEventAttributesGet(SaEvtEventHandleT eventHandle,
                                               SaEvtEventPatternArrayT
                                               *patternArray,
                                               SaUint8T *priority,
                                               SaTimeT * retentionTime,
                                               SaNameT *publisherName,
                                               SaTimeT * publishTime,
                                               SaEvtEventIdT * eventId);
    extern SaAisErrorT saEvtEventDataGet(SaEvtEventHandleT eventHandle,
                                         void *eventData,
                                         SaSizeT *eventDataSize);
    extern SaAisErrorT saEvtEventPublish(SaEvtEventHandleT eventHandle,
                                         const void *eventData,
                                         SaSizeT eventDataSize,
                                         SaEvtEventIdT * eventId);
    extern SaAisErrorT saEvtEventSubscribe(SaEvtChannelHandleT channelHandle,
                                           const SaEvtEventFilterArrayT
                                           *filters,
                                           SaEvtSubscriptionIdT subscriptionId);
    extern SaAisErrorT saEvtEventUnsubscribe(SaEvtChannelHandleT channelHandle,
                                             SaEvtSubscriptionIdT
                                             subscriptionId);
    extern SaAisErrorT saEvtEventRetentionTimeClear(SaEvtChannelHandleT
                                                    channelHandle,
                                                    SaEvtEventIdT eventId);

# ifdef  __cplusplus
}
# endif

#endif                          /* _SA_EVT_H */
