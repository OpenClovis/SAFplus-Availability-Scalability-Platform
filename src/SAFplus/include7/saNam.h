/*******************************************************************************
**
** FILE:
**   SaNam.h
**
** DESCRIPTION: 
**   This file provides the C language binding for the Service 
**   Availability(TM) Forum AIS Naming Service (NAM). It contains all of 
**   the prototypes and type definitions required for NAM. 
**   
** SPECIFICATION VERSION:
**   SAI-AIS-NAM-A.01.01
**
** DATE: 
**   Mon  Dec   28  2006  
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

#ifndef _SA_NAM_H
#define _SA_NAM_H

#include "saAis.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef SaUint64T SaNamHandleT;
typedef SaUint64T SaNamContextHandleT;
typedef SaUint64T SaNamContextIterationHandleT;

#define SA_NAM_CONTEXT_CLUSTER_DEFAULT "safNamContext=saNamContextClusterDefault"
#define SA_NAM_CONTEXT_NODE_DEFAULT "saNamContext=saNamContextNodeDefault"

typedef enum {
    SA_NAM_CONTEXT_SCOPE_CLUSTER = 1,
    SA_NAM_CONTEXT_SCOPE_NODE = 2
} SaNamContextScopeT;

typedef struct {
    SaTimeT retentionDuration;
    SaUint32T maxNumBindings;
    SaSizeT maxObjectSize;
} SaNamContextCreationAttributesT;

typedef enum {
    SA_NAM_BINDING_ADDED = 1,
    SA_NAM_BINDING_DELETED = 2,
    SA_NAM_BINDING_MODIFIED = 3
} SaNamBindingChangeT;

typedef struct {
    SaNameT *nameInBinding;
    void *objectInBinding;
    SaSizeT objectSize;
} SaNamBindingDescriptorT;

typedef enum {
    SA_NAM_ITERATE_NAME_ONLY = 1,
    SA_NAM_ITERATE_BINDING = 2
} SaNamBindingIterationOptionsT;

#define SA_NAM_CONTEXT_WRITER 0X1
#define SA_NAM_CONTEXT_READER 0X2
#define SA_NAM_CONTEXT_CREATE 0X4

typedef SaUint8T SaNamContextOpenFlagsT;

typedef void 
(*SaNamContextOpenCallbackT)(
    SaInvocationT invocation,
    SaNamContextHandleT contextHandle,
    SaAisErrorT error);

typedef void 
(*SaNamBindingUpdateCallbackT)(
    SaNamContextHandleT contextHandle,
    const SaNameT *nameBinding,
    SaNamBindingChangeT bindingChange);

typedef struct {
    SaNamContextOpenCallbackT saNamContextOpenCallback;
    SaNamBindingUpdateCallbackT saNamBindingUpdateCallback;
} SaNamCallbacksT;

typedef enum {
    SA_NAM_MAX_NUM_CLUSTER_CONTEXT_ID = 1,
    SA_NAM_MAX_NUM_NODE_CONTEXT_ID = 2
} SaNamLimitIdT;

typedef enum {
    SA_NAM_CONTEXT_CAPACITY_REACHED = 1,
    SA_NAM_CONTEXT_CAPACITY_AVAILABLE = 2
} SaNamCapacityStatusT;

typedef enum {
    SA_NAM_CONTEXT_CAPACITY_STATUS = 1
} SaNamStateT;

/*************************************************/
/******** NAM API function declarations **********/
/*************************************************/

extern SaAisErrorT 
saNamInitialize(
    SaNamHandleT *namHandle,
    const SaNamCallbacksT *namCallbacks,
    SaVersionT *version);

extern SaAisErrorT 
saNamSelectionObjectGet(
    SaNamHandleT namHandle,
    SaSelectionObjectT *selectionObject);

extern SaAisErrorT 
saNamDispatch(
    SaNamHandleT namHandle,
    SaDispatchFlagsT dispatchFlags);

extern SaAisErrorT 
saNamFinalize(
    SaNamHandleT namHandle);

extern SaAisErrorT 
saNamContextOpen(
    SaNamHandleT namHandle,
    const SaNameT *contextName,
    SaNamContextScopeT contextScope,
    const SaNamContextCreationAttributesT *contextCreationAttributes,
    SaNamContextOpenFlagsT contextOpenFlags,
    SaTimeT timeout,
    SaNamContextHandleT *contextHandle);

extern SaAisErrorT 
saNamContextOpenAsync(
    SaNamHandleT namHandle,
    SaInvocationT invocation,
    const SaNameT *contextName,
    SaNamContextScopeT contextScope,
    const SaNamContextCreationAttributesT *contextCreationAttributes,
    SaNamContextOpenFlagsT contextOpenFlags);

extern SaAisErrorT 
saNamContextClose(
    SaNamContextHandleT contextHandle);

extern SaAisErrorT 
saNamContextUnlink(
    SaNamHandleT namHandle,
    const SaNameT *contextName,
    SaNamContextScopeT contextScope);

extern SaAisErrorT 
saNamContextRetentionDurationSet(
    SaNamContextHandleT contextHandle,
    SaTimeT retentionDuration);

extern SaAisErrorT 
saNamBindingAdd(
    SaNamContextHandleT contextHandle,
    const SaNameT *nameInBinding,
    const void *objectToBind,
    SaSizeT objectSize);

extern SaAisErrorT 
saNamBindingLookup(
    SaNamContextHandleT contextHandle,
    const SaNameT *nameInBinding,
    void *objectBound,
    SaSizeT *objectSize);

extern SaAisErrorT 
saNamBindingModify(
    SaNamContextHandleT contextHandle,
    const SaNameT *nameInBinding,
    const void *objectToBind,
    SaSizeT objectSize);

extern SaAisErrorT 
saNamBindingCheckModify(
    SaNamContextHandleT contextHandle,
    const SaNameT *nameInBinding,
    const void *objectToBind,
    SaSizeT objectSize,
    const void *oldObjectBound,
    SaSizeT oldObjectSize);

extern SaAisErrorT 
saNamBindingDelete(
    SaNamContextHandleT contextHandle,
    const SaNameT *nameInBinding);

extern SaAisErrorT 
saNamBindingSubscribe(
    SaNamContextHandleT contextHandle,
    const SaNameT *nameInBinding);

extern SaAisErrorT 
saNamBindingUnsubscribe(
    SaNamContextHandleT contextHandle,
    const SaNameT *nameInBinding);

extern SaAisErrorT 
saNamContextIterationInitialize(
    SaNamContextHandleT contextHandle,
    SaNamBindingIterationOptionsT iterationOption,
    SaNamContextIterationHandleT *contextIterationHandle);

extern SaAisErrorT 
saNamContextIterationNext(
    SaNamContextIterationHandleT contextIterationHandle,
    SaNamBindingDescriptorT *bindingDescriptor);

extern SaAisErrorT 
saNamContextIterationFinalize(
    SaNamContextIterationHandleT contextIterationHandle);

extern SaAisErrorT 
saNamLimitGet(
    SaNamHandleT namHandle,
    SaNamLimitIdT limitId,
    SaLimitValueT *limitValue);

#ifdef  __cplusplus
}

#endif

#endif  /* _SA_NAM_H */
