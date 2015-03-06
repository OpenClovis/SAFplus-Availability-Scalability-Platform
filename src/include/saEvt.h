/*******************************************************************************
**
** FILE:
**   SaEvt.h
**
** DESCRIPTION: 
**   This file provides the C language binding for the Service 
**   Availability(TM) Forum AIS Event Service (EVT). It contains all of 
**   the prototypes and type definitions required for EVT. 
**   
** SPECIFICATION VERSION:
**   SAI-AIS-EVT-B.03.01
**
** DATE: 
**   Mon  Dec   18  2006  
**
** LEGAL:
**   OWNERSHIP OF SPECIFICATION AND COPYRIGHTS. 
**
** Copyright 2006 by the Service Availability Forum. All rights reserved.
**
** Permission to use, copy, modify, and distribute this software for any
** purpose without fee is hereby granted, provided that this entire notice
** is included in all copies of any software which is or includes a copy
** or modification of this software and in all copies of the supporting
** documentation for such software.
**
** THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
** WARRANTY.  IN PARTICULAR, THE SERVICE AVAILABILITY FORUM DOES NOT MAKE ANY
** REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
** OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
**
*******************************************************************************/

#ifndef _SA_EVT_H
#define _SA_EVT_H

#include "saAis.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef SaUint64T SaEvtHandleT;
typedef SaUint64T SaEvtEventHandleT;
typedef SaUint64T SaEvtChannelHandleT;
typedef SaUint32T SaEvtSubscriptionIdT;

typedef void 
(*SaEvtChannelOpenCallbackT)(
    SaInvocationT invocation,
    SaEvtChannelHandleT channelHandle,
    SaAisErrorT error);

typedef void
(*SaEvtEventDeliverCallbackT)(
    SaEvtSubscriptionIdT subscriptionId,
    SaEvtEventHandleT eventHandle,
    SaSizeT eventDataSize);

typedef struct {
    SaEvtChannelOpenCallbackT  saEvtChannelOpenCallback; 
    SaEvtEventDeliverCallbackT saEvtEventDeliverCallback; 
} SaEvtCallbacksT;

#define SA_EVT_CHANNEL_PUBLISHER  0x1
#define SA_EVT_CHANNEL_SUBSCRIBER 0x2
#define SA_EVT_CHANNEL_CREATE     0x4
typedef SaUint8T SaEvtChannelOpenFlagsT;

typedef struct {
    SaSizeT   allocatedSize;
    SaSizeT   patternSize;
    SaUint8T *pattern;
} SaEvtEventPatternT;

typedef struct {
    SaSizeT             allocatedNumber;
    SaSizeT             patternsNumber;
    SaEvtEventPatternT *patterns;
} SaEvtEventPatternArrayT;

#define SA_EVT_HIGHEST_PRIORITY 0
#define SA_EVT_LOWEST_PRIORITY  3
typedef SaUint8T SaEvtEventPriorityT;

typedef SaUint64T SaEvtEventIdT;
#define SA_EVT_EVENTID_NONE 0LL
#define SA_EVT_EVENTID_LOST 1LL

#define SA_EVT_LOST_EVENT "SA_EVT_LOST_EVENT_PATTERN"

typedef enum {
    SA_EVT_PREFIX_FILTER = 1,
    SA_EVT_SUFFIX_FILTER = 2,
    SA_EVT_EXACT_FILTER = 3,
    SA_EVT_PASS_ALL_FILTER = 4
} SaEvtEventFilterTypeT;

typedef struct {
    SaEvtEventFilterTypeT filterType;
    SaEvtEventPatternT    filter;
} SaEvtEventFilterT;

typedef struct {
    SaSizeT            filtersNumber;
    SaEvtEventFilterT *filters;
} SaEvtEventFilterArrayT;

typedef enum {
    SA_EVT_MAX_NUM_CHANNELS_ID = 1,
    SA_EVT_MAX_EVT_SIZE_ID = 2,
    SA_EVT_MAX_PATTERN_SIZE_ID = 3,
    SA_EVT_MAX_NUM_PATTERNS_ID = 4,
    SA_EVT_MAX_RETENTION_DURATION_ID = 5,
} SaEvtLimitIdT;

/*************************************************/
/******** EVT API function declarations **********/
/*************************************************/
extern SaAisErrorT 
saEvtInitialize(
    SaEvtHandleT *evtHandle, 
    const SaEvtCallbacksT *callbacks,
    SaVersionT *version);

extern SaAisErrorT 
saEvtSelectionObjectGet(
    SaEvtHandleT evtHandle,
    SaSelectionObjectT *selectionObject);

extern SaAisErrorT 
saEvtDispatch(
    SaEvtHandleT evtHandle, 
    SaDispatchFlagsT dispatchFlags);

extern SaAisErrorT 
saEvtFinalize(
    SaEvtHandleT evtHandle);

extern SaAisErrorT 
saEvtChannelOpen(
    SaEvtHandleT evtHandle, 
    const SaNameT *channelName,
    SaEvtChannelOpenFlagsT channelOpenFlags,
    SaTimeT timeout,
    SaEvtChannelHandleT *channelHandle);

extern SaAisErrorT 
saEvtChannelOpenAsync(
    SaEvtHandleT evtHandle, 
    SaInvocationT invocation,
    const SaNameT *channelName,
    SaEvtChannelOpenFlagsT channelOpenFlags);

extern SaAisErrorT 
saEvtChannelClose(
    SaEvtChannelHandleT channelHandle);

extern SaAisErrorT 
saEvtChannelUnlink(
    SaEvtHandleT evtHandle, const SaNameT *channelName);

extern SaAisErrorT 
saEvtEventAllocate(
    SaEvtChannelHandleT channelHandle,
    SaEvtEventHandleT *eventHandle);

extern SaAisErrorT 
saEvtEventFree(
    SaEvtEventHandleT eventHandle);

extern SaAisErrorT 
saEvtEventAttributesSet(
    SaEvtEventHandleT eventHandle,
    const SaEvtEventPatternArrayT *patternArray,
    SaUint8T priority,
    SaTimeT retentionTime,
    const SaNameT *publisherName);

extern SaAisErrorT 
saEvtEventAttributesGet(
    SaEvtEventHandleT eventHandle,
    SaEvtEventPatternArrayT *patternArray,
    SaUint8T *priority,
    SaTimeT  *retentionTime,
    SaNameT  *publisherName,
    SaTimeT  *publishTime,
    SaEvtEventIdT *eventId);

extern SaAisErrorT 
saEvtEventPatternFree(
    SaEvtEventHandleT eventHandle,
    SaEvtEventPatternT *patterns);

extern SaAisErrorT 
saEvtEventDataGet(
    SaEvtEventHandleT eventHandle,
    void *eventData,
    SaSizeT *eventDataSize);

extern SaAisErrorT 
saEvtEventPublish(
    SaEvtEventHandleT eventHandle,
    const void *eventData,
    SaSizeT eventDataSize,
    SaEvtEventIdT *eventId);

extern SaAisErrorT 
saEvtEventSubscribe(
    SaEvtChannelHandleT channelHandle,
    const SaEvtEventFilterArrayT *filters,
    SaEvtSubscriptionIdT subscriptionId);

extern SaAisErrorT 
saEvtEventUnsubscribe(
    SaEvtChannelHandleT channelHandle,
    SaEvtSubscriptionIdT subscriptionId);

extern SaAisErrorT 
saEvtEventRetentionTimeClear(
    SaEvtChannelHandleT channelHandle,
    SaEvtEventIdT eventId);

extern SaAisErrorT 
saEvtLimitGet(
    SaEvtHandleT evtHandle,
    SaEvtLimitIdT limitId,
    SaLimitValueT *limitValue);

#ifdef  __cplusplus
}
#endif

#endif  /* _SA_EVT_H */

