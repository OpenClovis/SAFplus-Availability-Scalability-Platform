/*******************************************************************************
**
** FILE:
**   SaTmr.h
**
** DESCRIPTION: 
**   This file provides the C language binding for the Service 
**   Availability(TM) Forum AIS Timer Service (TMR). It contains  
**   all the prototypes and type definitions required for TMR. 
**   
** SPECIFICATION VERSION:
**   SAI-AIS-TMR-A.01.01
**
** DATE: 
**   Mon  Dec   18  2006  
**
** LEGAL:
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


#ifndef _SA_TMR_H
#define _SA_TMR_H

#include "saAis.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef SaUint64T SaTmrHandleT;
typedef SaUint32T SaTmrTimerIdT;

typedef enum {
    SA_TIME_ABSOLUTE = 1,
    SA_TIME_DURATION = 2
} SaTmrTimeTypeT;

typedef struct {
    SaTmrTimeTypeT type;
    SaTimeT        initialExpirationTime;
    SaTimeT        initialPeriodDuration;
} SaTmrTimerAttributesT;

typedef void 
(*SaTmrTimerExpiredCallbackT)(
    SaTmrTimerIdT timerId,
    const void *timerData,
    SaUint32T expirationCount);

typedef struct {
    SaTmrTimerExpiredCallbackT saTmrTimerExpiredCallback;
} SaTmrCallbacksT;

/*************************************************/
/******** TMR API function declarations **********/
/*************************************************/

extern SaAisErrorT 
saTmrInitialize(
    SaTmrHandleT *timerHandle,
    const SaTmrCallbacksT *timerCallbacks,
    SaVersionT *version);

extern SaAisErrorT 
saTmrSelectionObjectGet(
    SaTmrHandleT timerHandle,
    SaSelectionObjectT *selectionObject);

extern SaAisErrorT
saTmrDispatch(
    SaTmrHandleT timerHandle,
    SaDispatchFlagsT dispatchFlags);
              
extern SaAisErrorT 
saTmrFinalize(
    SaTmrHandleT timerHandle);

extern SaAisErrorT
saTmrTimerStart(
    SaTmrHandleT timerHandle,
    const SaTmrTimerAttributesT *timerAttributes,
    const void *timerData,
    SaTmrTimerIdT *timerId,
    SaTimeT *callTime); 
                
extern SaAisErrorT 
saTmrTimerReschedule(
    SaTmrHandleT timerHandle,
    SaTmrTimerIdT timerId,
    const SaTmrTimerAttributesT *timerAttributes,
    SaTimeT *callTime);  
                       
extern SaAisErrorT 
saTmrTimerCancel(
    SaTmrHandleT timerHandle,
    SaTmrTimerIdT timerId,
    void **timerDataP);    
    
extern SaAisErrorT
saTmrPeriodicTimerSkip(
    SaTmrHandleT timerHandle,
    SaTmrTimerIdT timerId);  
                          
extern SaAisErrorT 
saTmrTimerRemainingTimeGet(
    SaTmrHandleT timerHandle,
    SaTmrTimerIdT timerId,
    SaTimeT *remainingTime);    
    
extern SaAisErrorT
saTmrTimerAttributesGet(
    SaTmrHandleT timerHandle,
    SaTmrTimerIdT timerId,
    SaTmrTimerAttributesT *timerAttributes); 
                            
extern SaAisErrorT 
saTmrTimeGet(
    SaTmrHandleT timerHandle,
    SaTimeT *currentTime); 
                
extern SaAisErrorT 
saTmrClockTickGet(
    SaTmrHandleT timerHandle,
    SaTimeT *clockTick);    

#ifdef  __cplusplus
}
#endif

#endif  /* _SA_TMR_H */
