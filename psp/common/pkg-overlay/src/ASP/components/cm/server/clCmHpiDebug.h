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
/******************************************************************************
 * Description :
 * This header file is for the HPI API debug shim layer
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
#ifndef _CL_CM_HPI_DEBUG_H_
#define _CL_CM_HPI_DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#if defined(HCL_CM)
#include <SaHpi.h>
#else
#include <SaHpi.h>
#endif

/******************************************************************************
 * Shim layer function prototypes
 *****************************************************************************/

#ifndef CL_CM_HPI_CALL_DEBUG

#define _saHpiVersionGet                saHpiVersionGet
#define _saHpiSessionOpen               saHpiSessionOpen
#define _saHpiSessionClose              saHpiSessionClose
#define _saHpiDiscover                  saHpiDiscover
#define _saHpiDrtEntryGet               saHpiDrtEntryGet
#define _saHpiRptEntryGet               saHpiRptEntryGet
#define _saHpiRptEntryGetByResourceId   saHpiRptEntryGetByResourceId
#define _saHpiSubscribe                 saHpiSubscribe
#define _saHpiUnsubscribe               saHpiUnsubscribe
#define _saHpiEventGet                  saHpiEventGet
#define _saHpiRdrGet                    saHpiRdrGet
#define _saHpiHotSwapPolicyCancel       saHpiHotSwapPolicyCancel
#define _saHpiResourceActiveSet         saHpiResourceActiveSet
#define _saHpiResourceInactiveSet       saHpiResourceInactiveSet
#define _saHpiAutoInsertTimeoutGet      saHpiAutoInsertTimeoutGet
#define _saHpiAutoInsertTimeoutSet      saHpiAutoInsertTimeoutSet
#define _saHpiAutoExtractTimeoutGet     saHpiAutoExtractTimeoutGet
#define _saHpiAutoExtractTimeoutSet     saHpiAutoExtractTimeoutSet
#define _saHpiHotSwapStateGet           saHpiHotSwapStateGet
#define _saHpiHotSwapActionRequest      saHpiHotSwapActionRequest
#define _saHpiHotSwapIndicatorStateSet  saHpiHotSwapIndicatorStateSet
#define _saHpiResourceResetStateSet     saHpiResourceResetStateSet
#define _saHpiResourcePowerStateSet     saHpiResourcePowerStateSet
#define _saHpiSensorReadingGet          saHpiSensorReadingGet

#else /* CL_CM_HPI_CALL_DEBUG */

SaHpiVersionT SAHPI_API _saHpiVersionGet ( void );

SaErrorT SAHPI_API _saHpiSessionOpen (
    SAHPI_IN  SaHpiDomainIdT        DomainId,
    SAHPI_OUT SaHpiSessionIdT       *SessionId,
    SAHPI_IN  void                  *SecurityParams
);

SaErrorT SAHPI_API _saHpiSessionClose (
    SAHPI_IN SaHpiSessionIdT        SessionId
);

SaErrorT SAHPI_API _saHpiDiscover (
    SAHPI_IN SaHpiSessionIdT        SessionId  
);

SaErrorT SAHPI_API _saHpiDrtEntryGet (
    SAHPI_IN  SaHpiSessionIdT       SessionId,
    SAHPI_IN  SaHpiEntryIdT         EntryId,
    SAHPI_OUT SaHpiEntryIdT         *NextEntryId,
    SAHPI_OUT SaHpiDrtEntryT        *DrtEntry
);

SaErrorT SAHPI_API _saHpiRptEntryGet (
    SAHPI_IN  SaHpiSessionIdT       SessionId,
    SAHPI_IN  SaHpiEntryIdT         EntryId,
    SAHPI_OUT SaHpiEntryIdT         *NextEntryId,
    SAHPI_OUT SaHpiRptEntryT        *RptEntry
);

SaErrorT SAHPI_API _saHpiRptEntryGetByResourceId (
    SAHPI_IN  SaHpiSessionIdT       SessionId,
    SAHPI_IN  SaHpiResourceIdT      ResourceId,
    SAHPI_OUT SaHpiRptEntryT        *RptEntry
);

SaErrorT SAHPI_API _saHpiSubscribe (
    SAHPI_IN SaHpiSessionIdT        SessionId
);

SaErrorT SAHPI_API _saHpiUnsubscribe (
    SAHPI_IN SaHpiSessionIdT        SessionId
);

SaErrorT SAHPI_API _saHpiEventGet (
    SAHPI_IN SaHpiSessionIdT        SessionId,
    SAHPI_IN SaHpiTimeoutT          Timeout,
    SAHPI_OUT SaHpiEventT           *Event,
    SAHPI_INOUT SaHpiRdrT           *Rdr,
    SAHPI_INOUT SaHpiRptEntryT      *RptEntry,
    SAHPI_INOUT SaHpiEvtQueueStatusT *EventQueueStatus
);

SaErrorT SAHPI_API _saHpiRdrGet (
    SAHPI_IN  SaHpiSessionIdT       SessionId,
    SAHPI_IN  SaHpiResourceIdT      ResourceId,
    SAHPI_IN  SaHpiEntryIdT         EntryId,
    SAHPI_OUT SaHpiEntryIdT         *NextEntryId,
    SAHPI_OUT SaHpiRdrT             *Rdr
);

SaErrorT SAHPI_API _saHpiHotSwapPolicyCancel (
    SAHPI_IN SaHpiSessionIdT        SessionId,
    SAHPI_IN SaHpiResourceIdT       ResourceId
);

SaErrorT SAHPI_API _saHpiResourceActiveSet (
    SAHPI_IN SaHpiSessionIdT        SessionId,
    SAHPI_IN SaHpiResourceIdT       ResourceId
);

SaErrorT SAHPI_API _saHpiResourceInactiveSet (
    SAHPI_IN SaHpiSessionIdT        SessionId,
    SAHPI_IN SaHpiResourceIdT       ResourceId
);

SaErrorT SAHPI_API _saHpiAutoInsertTimeoutGet( 
    SAHPI_IN SaHpiSessionIdT        SessionId,
    SAHPI_IN SaHpiTimeoutT          *Timeout
);

SaErrorT SAHPI_API _saHpiAutoInsertTimeoutSet( 
    SAHPI_IN SaHpiSessionIdT        SessionId,
    SAHPI_IN SaHpiTimeoutT          Timeout
);

SaErrorT SAHPI_API _saHpiAutoExtractTimeoutGet(
    SAHPI_IN  SaHpiSessionIdT       SessionId,
    SAHPI_IN  SaHpiResourceIdT      ResourceId,
    SAHPI_IN  SaHpiTimeoutT         *Timeout
);

SaErrorT SAHPI_API _saHpiAutoExtractTimeoutSet(
    SAHPI_IN  SaHpiSessionIdT       SessionId,
    SAHPI_IN  SaHpiResourceIdT      ResourceId,
    SAHPI_IN  SaHpiTimeoutT         Timeout
);

SaErrorT SAHPI_API _saHpiHotSwapStateGet (
    SAHPI_IN  SaHpiSessionIdT       SessionId,
    SAHPI_IN  SaHpiResourceIdT      ResourceId,
    SAHPI_OUT SaHpiHsStateT         *State
);

SaErrorT SAHPI_API _saHpiHotSwapActionRequest (
    SAHPI_IN SaHpiSessionIdT        SessionId,
    SAHPI_IN SaHpiResourceIdT       ResourceId,
    SAHPI_IN SaHpiHsActionT         Action
);

SaErrorT SAHPI_API _saHpiHotSwapIndicatorStateSet (
    SAHPI_IN SaHpiSessionIdT        SessionId,
    SAHPI_IN SaHpiResourceIdT       ResourceId,
    SAHPI_IN SaHpiHsIndicatorStateT State
);

SaErrorT SAHPI_API _saHpiResourceResetStateSet (
    SAHPI_IN SaHpiSessionIdT        SessionId,
    SAHPI_IN SaHpiResourceIdT       ResourceId,
    SAHPI_IN SaHpiResetActionT      ResetAction
);

SaErrorT SAHPI_API _saHpiResourcePowerStateSet (
    SAHPI_IN SaHpiSessionIdT        SessionId,
    SAHPI_IN SaHpiResourceIdT       ResourceId,
    SAHPI_IN SaHpiPowerStateT       State
);

SaErrorT SAHPI_API _saHpiSensorReadingGet (
    SAHPI_IN    SaHpiSessionIdT       SessionId,
    SAHPI_IN    SaHpiResourceIdT      ResourceId,
    SAHPI_IN    SaHpiSensorNumT       SensorNum,
    SAHPI_INOUT SaHpiSensorReadingT   *Reading,
    SAHPI_INOUT SaHpiEventStateT      *EventState
);

#endif /* CL_CM_HPI_CALL_DEBUG */

#ifdef __cplusplus
}
#endif

#endif /* _CL_CM_HPI_DEBUG_H_ */
