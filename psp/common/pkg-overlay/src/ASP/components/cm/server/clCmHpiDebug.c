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
 * This is the HPI API debug shim layer
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/

/* Moving the includes outside #ifdef so that compiler does not give a warning of empty file */
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#ifdef CL_CM_HPI_CALL_DEBUG


#include <SaHpi.h>
#include <oh_utils.h>

#include <include/clCmDefs.h> /* this is ugly */

#include "clCmHpiDebug.h"

#define _PREFIX "[CM] HPI "
#define _DEBUG_FILE stdout
#define _STRUCT_OFFSET 12

#define CL_CM_HPI_DEBUG_PRINT(ARGS) \
    { \
        printf ARGS; \
        fflush(stdout); \
    }

static char *bool_str[2] = { "False", "True" };

/*****************************************************************************/
SaHpiVersionT SAHPI_API _saHpiVersionGet ( void )
{
    SaHpiVersionT version;
    static const char *ver_str[8] = {
        "unknonw", "A", "B", "C", "D", "E", "F", "G"
    };
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiVersionGet() called\n"
    ));

    version = saHpiVersionGet();
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiVersionGet() returned with:\n"
        _PREFIX "        Version =        %s.%02d.%02d\n",
        ver_str[((version >> 16) & 0x7)],
        (int)((version >> 8) & 0xff),
        (int)(version & 0xff)
    ));

    return version;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiSessionOpen (
    SAHPI_IN  SaHpiDomainIdT      DomainId,
    SAHPI_OUT SaHpiSessionIdT     *SessionId,
    SAHPI_IN  void                *SecurityParams
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiSessionOpen() called with:\n"
        _PREFIX "        DomainId =       %d\n"
        _PREFIX "        SecurityParams = 0x%p\n",
        (int)DomainId,
        SecurityParams
    ));

    saError = saHpiSessionOpen (DomainId, SessionId, SecurityParams);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiSessionOpen() returned with:\n"
        _PREFIX "        Status =         %d (%s)\n"
        _PREFIX "        SessionId =      %d\n",
        saError, oh_lookup_error(saError),
        (int)*SessionId
    ));

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiSessionClose (
    SAHPI_IN SaHpiSessionIdT SessionId
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiSessionClose() called with:\n"
        _PREFIX "        SessionId =      %d\n",
        (int)SessionId
    ));

    saError = saHpiSessionClose (SessionId);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiSessionClose() returned with:\n"
        _PREFIX "        status =         %d (%s)\n",
        saError, oh_lookup_error(saError)
    ));

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiDiscover (
    SAHPI_IN SaHpiSessionIdT SessionId
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiDiscover() called with:\n"
        _PREFIX "        SessionId =      %d\n",
        (int)SessionId
    ));

    saError = saHpiDiscover (SessionId);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiDiscover() returned with:\n"
        _PREFIX "        status =         %d (%s)\n",
        saError, oh_lookup_error(saError)
    ));

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiDrtEntryGet (
    SAHPI_IN  SaHpiSessionIdT     SessionId,
    SAHPI_IN  SaHpiEntryIdT       EntryId,
    SAHPI_OUT SaHpiEntryIdT       *NextEntryId,
    SAHPI_OUT SaHpiDrtEntryT      *DrtEntry
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiDrtEntryGet() called with:\n"
        _PREFIX "        SessionId =      %d\n"
        _PREFIX "        EntryId =        0x%x\n",
        (int)SessionId,
        EntryId
    ));

    saError = saHpiDrtEntryGet (SessionId, EntryId, NextEntryId, DrtEntry);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiDrtEntryGet() returned with:\n"
        _PREFIX "        Status =         %d (%s)\n"
        _PREFIX "        NextEntryId =    0x%x\n"
        _PREFIX "        DrtEntry = \n"
        _PREFIX "                EntryId =  0x%x\n"
        _PREFIX "                DomainId = %d\n"
        _PREFIX "                IsPeer =   %s\n",
        saError, oh_lookup_error(saError),
        *NextEntryId,
        DrtEntry->EntryId,
        (int)DrtEntry->DomainId,
        bool_str[!(!(DrtEntry->IsPeer))]
    ));

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiRptEntryGet (
    SAHPI_IN  SaHpiSessionIdT     SessionId,
    SAHPI_IN  SaHpiEntryIdT       EntryId,
    SAHPI_OUT SaHpiEntryIdT       *NextEntryId,
    SAHPI_OUT SaHpiRptEntryT      *RptEntry
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiRptEntryGet() called with:\n"
        _PREFIX "        SessionId =      %d\n"
        _PREFIX "        EntryId =        0x%x\n",
        (int)SessionId,
        EntryId
    ));

    saError = saHpiRptEntryGet (SessionId, EntryId, NextEntryId, RptEntry);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiRptEntryGet() returned with:\n"
        _PREFIX "        Status =         %d (%s)\n"
        _PREFIX "        NextEntryId =    0x%x\n"
        _PREFIX "        RptEntry = \n",
        saError, oh_lookup_error(saError),
        *NextEntryId
    ));
    (void)oh_fprint_rptentry(_DEBUG_FILE, RptEntry, _STRUCT_OFFSET);
    fflush(stdout);
    
    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiRptEntryGetByResourceId (
    SAHPI_IN  SaHpiSessionIdT      SessionId,
    SAHPI_IN  SaHpiResourceIdT     ResourceId,
    SAHPI_OUT SaHpiRptEntryT       *RptEntry
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiRptEntryGetByResourceId() called with:\n"
        _PREFIX "        SessionId =      %d\n"
        _PREFIX "        ResourceId =     0x%x (%d)\n",
        (int)SessionId,
        (unsigned int)ResourceId, (int)ResourceId
    ));

    saError = saHpiRptEntryGetByResourceId (SessionId, ResourceId, RptEntry);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiRptEntryGetByResourceId() returned with:\n"
        _PREFIX "        Status =         %d (%s)\n"
        _PREFIX "        RptEntry = \n",
        saError, oh_lookup_error(saError)
    ));
    (void)oh_fprint_rptentry(_DEBUG_FILE, RptEntry, _STRUCT_OFFSET);
    fflush(stdout);

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiSubscribe (
    SAHPI_IN SaHpiSessionIdT      SessionId
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiSubscribe() called with:\n"
        _PREFIX "        SessionId =      %d\n",
        (int)SessionId
    ));

    saError = saHpiSubscribe (SessionId);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiSubscribe() returned with:\n"
        _PREFIX "        status =         %d (%s)\n",
        saError, oh_lookup_error(saError)
    ));

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiUnsubscribe (
    SAHPI_IN SaHpiSessionIdT  SessionId
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiUnsubscribe() called with:\n"
        _PREFIX "        SessionId =      %d\n",
        (int)SessionId
    ));

    saError = saHpiUnsubscribe (SessionId);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiUnsubscribe() returned with:\n"
        _PREFIX "        status =         %d (%s)\n",
        saError, oh_lookup_error(saError)
    ));

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiEventGet (
    SAHPI_IN SaHpiSessionIdT            SessionId,
    SAHPI_IN SaHpiTimeoutT              Timeout,
    SAHPI_OUT SaHpiEventT               *Event,
    SAHPI_INOUT SaHpiRdrT               *Rdr,
    SAHPI_INOUT SaHpiRptEntryT          *RptEntry,
    SAHPI_INOUT SaHpiEvtQueueStatusT    *EventQueueStatus
)
{
    SaErrorT saError;
    struct timeval tv_now;
    struct tm tm_now;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiEventGet() called with:\n"
        _PREFIX "        SessionId =      %d\n"
        _PREFIX "        Timeout =        %lld [ns] (-1 denotes blocking/infinite)\n"
        _PREFIX "        Rdr =            0x%p\n"
        _PREFIX "        RptEntry =       0x%p\n"
        _PREFIX "        EventQueueStatus = 0x%p\n",
        (int)SessionId,
        Timeout,
        (void*)Rdr,
        (void*)RptEntry,
        (void*)EventQueueStatus
    ));

    saError = saHpiEventGet (SessionId, Timeout, Event, Rdr, RptEntry,
                             EventQueueStatus);
    
    (void)gettimeofday(&tv_now, NULL);
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiEventGet() returned at %s with:\n"
        _PREFIX "        status =         %d (%s)\n"
        _PREFIX "        Event = \n",
        asctime(localtime_r(&tv_now.tv_sec, &tm_now)),
        saError, oh_lookup_error(saError)
    ));

/*
 * Assume that we use the 2.8.1 version of openhpi. This is fair because configure
 * script check for this exact version
 */
/*
 * #if OPENHPI_VERSION >= 020600
 */
    (void)oh_fprint_event(_DEBUG_FILE, Event, &(Rdr->Entity), _STRUCT_OFFSET);
/*
 * #else
 *     (void)oh_fprint_event(_DEBUG_FILE, Event, _STRUCT_OFFSET);
 * #endif
 */
    fflush(stdout);

    if (Rdr != NULL)
    {
        CL_CM_HPI_DEBUG_PRINT((
            _PREFIX "        Rdr = \n"
        ));
        (void)oh_fprint_rdr(_DEBUG_FILE, Rdr, _STRUCT_OFFSET);
        fflush(stdout);
    }
    
    if (RptEntry != NULL)
    {
        CL_CM_HPI_DEBUG_PRINT((
            _PREFIX "        RptEntry = \n"
        ));
        (void)oh_fprint_rptentry(_DEBUG_FILE, RptEntry, _STRUCT_OFFSET);
        fflush(stdout);
    }
    
    if (EventQueueStatus != NULL)
    {
        CL_CM_HPI_DEBUG_PRINT((
            _PREFIX "        EventQueueStatus = %d (1 indicates overflow)\n",
            (int)*EventQueueStatus
        ));
    }
    
    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiRdrGet (
    SAHPI_IN  SaHpiSessionIdT       SessionId,
    SAHPI_IN  SaHpiResourceIdT      ResourceId,
    SAHPI_IN  SaHpiEntryIdT         EntryId,
    SAHPI_OUT SaHpiEntryIdT         *NextEntryId,
    SAHPI_OUT SaHpiRdrT             *Rdr
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiRdrGet() called with:\n"
        _PREFIX "        SessionId =      %d\n"
        _PREFIX "        ResourceId =     0x%x (%d)\n"
        _PREFIX "        EntryId =        0x%x\n",
        (int)SessionId,
        (unsigned int)ResourceId, (int)ResourceId,
        (unsigned int)EntryId
    ));

    saError = saHpiRdrGet (SessionId, ResourceId, EntryId, NextEntryId, Rdr);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiRdrGet() returned with:\n"
        _PREFIX "        status =         %d (%s)\n"
        _PREFIX "        NextEntryId =    0x%x\n"
        _PREFIX "        Rdr = \n",
        saError, oh_lookup_error(saError),
        *NextEntryId
    ));
    (void)oh_fprint_rdr(_DEBUG_FILE, Rdr, _STRUCT_OFFSET);
    fflush(stdout);

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiHotSwapPolicyCancel (
    SAHPI_IN SaHpiSessionIdT      SessionId,
    SAHPI_IN SaHpiResourceIdT     ResourceId
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiHotSwapPolicyCancel() called with:\n"
        _PREFIX "        SessionId =      %d\n"
        _PREFIX "        ResourceId =     0x%x (%d)\n",
        (int)SessionId,
        (unsigned int)ResourceId, (int)ResourceId
    ));

    saError = saHpiHotSwapPolicyCancel (SessionId, ResourceId);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiHotSwapPolicyCancel() returned with:\n"
        _PREFIX "        status =         %d (%s)\n",
        saError, oh_lookup_error(saError)
    ));

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiResourceActiveSet (
    SAHPI_IN SaHpiSessionIdT     SessionId,
    SAHPI_IN SaHpiResourceIdT    ResourceId
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiResourceActiveSet() called with:\n"
        _PREFIX "        SessionId =      %d\n"
        _PREFIX "        ResourceId =     0x%x (%d)\n",
        (int)SessionId,
        (unsigned int)ResourceId, (int)ResourceId
    ));

    saError = saHpiResourceActiveSet (SessionId, ResourceId);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiResourceActiveSet() returned with:\n"
        _PREFIX "        status =         %d (%s)\n",
        saError, oh_lookup_error(saError)
    ));

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiResourceInactiveSet (
    SAHPI_IN SaHpiSessionIdT     SessionId,
    SAHPI_IN SaHpiResourceIdT    ResourceId
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiResourceInactiveSet() called with:\n"
        _PREFIX "        SessionId =      %d\n"
        _PREFIX "        ResourceId =     0x%x (%d)\n",
        (int)SessionId,
        (unsigned int)ResourceId, (int)ResourceId
    ));

    saError = saHpiResourceInactiveSet (SessionId, ResourceId);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiResourceInactiveSet() returned with:\n"
        _PREFIX "        status =         %d (%s)\n",
        saError, oh_lookup_error(saError)
    ));

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiAutoInsertTimeoutSet (
    SAHPI_IN SaHpiSessionIdT        SessionId,
    SAHPI_IN SaHpiTimeoutT          Timeout
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiAutoInsertTimeoutSet() called with:\n"
        _PREFIX "        SessionId =      %d\n"
        _PREFIX "        Timeout =        %lld [ns]\n",
        (int)SessionId,
        Timeout
    ));

    saError = saHpiAutoInsertTimeoutSet (SessionId, Timeout);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiAutoInsertTimeoutSet() returned with:\n"
        _PREFIX "        status =         %d (%s)\n",
        saError, oh_lookup_error(saError)
    ));

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiAutoInsertTimeoutGet (
    SAHPI_IN SaHpiSessionIdT        SessionId,
    SAHPI_IN SaHpiTimeoutT          *Timeout
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiAutoInsertTimeoutGet() called with:\n"
        _PREFIX "        SessionId =      %d\n",
        (int)SessionId
    ));

    saError = saHpiAutoInsertTimeoutGet (SessionId, Timeout);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiAutoInsertTimeoutGet() returned with:\n"
        _PREFIX "        status =         %d (%s)\n"
        _PREFIX "        Timeout =        %lld [ns]\n",
        saError, oh_lookup_error(saError),
        *Timeout
    ));

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiAutoExtractTimeoutGet (
    SAHPI_IN  SaHpiSessionIdT      SessionId,
    SAHPI_IN  SaHpiResourceIdT     ResourceId,
    SAHPI_IN  SaHpiTimeoutT        *Timeout
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiAutoExtractTimeoutGet() called with:\n"
        _PREFIX "        SessionId =      %d\n"
        _PREFIX "        ResourceId =     0x%x (%d)\n",
        (int)SessionId,
        (unsigned int)ResourceId, (int)ResourceId
    ));

    saError = saHpiAutoExtractTimeoutGet (SessionId, ResourceId, Timeout);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiAutoExtractTimeoutGet() returned with:\n"
        _PREFIX "        status =         %d (%s)\n"
        _PREFIX "        Timeout =        %lld [ns]\n",
        saError, oh_lookup_error(saError),
        *Timeout
    ));

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiAutoExtractTimeoutSet (
    SAHPI_IN  SaHpiSessionIdT      SessionId,
    SAHPI_IN  SaHpiResourceIdT     ResourceId,
    SAHPI_IN  SaHpiTimeoutT        Timeout
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiAutoExtractTimeoutSet() called with:\n"
        _PREFIX "        SessionId =      %d\n"
        _PREFIX "        ResourceId =     0x%x (%d)\n"
        _PREFIX "        Timeout =        %lld [ns]\n",
        (int)SessionId,
        (unsigned int)ResourceId, (int)ResourceId,
        Timeout
    ));

    saError = saHpiAutoExtractTimeoutSet (SessionId, ResourceId, Timeout);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiAutoExtractTimeoutSet() returned with:\n"
        _PREFIX "        status =         %d (%s)\n",
        saError, oh_lookup_error(saError)
    ));

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiHotSwapStateGet (
    SAHPI_IN  SaHpiSessionIdT       SessionId,
    SAHPI_IN  SaHpiResourceIdT      ResourceId,
    SAHPI_OUT SaHpiHsStateT         *State
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiHotSwapStateGet() called with:\n"
        _PREFIX "        SessionId =      %d\n"
        _PREFIX "        ResourceId =     0x%x (%d)\n",
        (int)SessionId,
        (unsigned int)ResourceId, (int)ResourceId
    ));

    saError = saHpiHotSwapStateGet (SessionId, ResourceId, State);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiHotSwapStateGet() returned with:\n"
        _PREFIX "        status =         %d (%s)\n"
        _PREFIX "        State =          %s\n",
        saError, oh_lookup_error(saError),
        oh_lookup_hsstate(*State)
    ));

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiHotSwapActionRequest (
    SAHPI_IN SaHpiSessionIdT     SessionId,
    SAHPI_IN SaHpiResourceIdT    ResourceId,
    SAHPI_IN SaHpiHsActionT      Action
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiHotSwapActionRequest() called with:\n"
        _PREFIX "        SessionId =      %d\n"
        _PREFIX "        ResourceId =     0x%x (%d)\n"
        _PREFIX "        Action =         %s\n",
        (int)SessionId,
        (unsigned int)ResourceId, (int)ResourceId,
        oh_lookup_hsaction(Action)
    ));

    saError = saHpiHotSwapActionRequest (SessionId, ResourceId, Action);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiHotSwapStateGet() returned with:\n"
        _PREFIX "        status =         %d (%s)\n",
        saError, oh_lookup_error(saError)
    ));

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiHotSwapIndicatorStateSet (
    SAHPI_IN SaHpiSessionIdT           SessionId,
    SAHPI_IN SaHpiResourceIdT          ResourceId,
    SAHPI_IN SaHpiHsIndicatorStateT    State
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiHotSwapIndicatorStateSet() called with:\n"
        _PREFIX "        SessionId =      %d\n"
        _PREFIX "        ResourceId =     0x%x (%d)\n"
        _PREFIX "        State =          %s\n",
        (int)SessionId,
        (unsigned int)ResourceId, (int)ResourceId,
        oh_lookup_hsindicatorstate(State)
    ));

    saError = saHpiHotSwapIndicatorStateSet (SessionId, ResourceId, State);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiHotSwapIndicatorStateSet() returned with:\n"
        _PREFIX "        status =         %d (%s)\n",
        saError, oh_lookup_error(saError)
    ));

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiResourceResetStateSet (
    SAHPI_IN SaHpiSessionIdT       SessionId,
    SAHPI_IN SaHpiResourceIdT      ResourceId,
    SAHPI_IN SaHpiResetActionT     ResetAction
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiResourceResetStateSet() called with:\n"
        _PREFIX "        SessionId =      %d\n"
        _PREFIX "        ResourceId =     0x%x (%d)\n"
        _PREFIX "        ResetAction =    %s\n",
        (int)SessionId,
        (unsigned int)ResourceId, (int)ResourceId,
        oh_lookup_resetaction(ResetAction)
    ));

    saError = saHpiResourceResetStateSet (SessionId, ResourceId, ResetAction);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiResourceResetStateSet() returned with:\n"
        _PREFIX "        status =         %d (%s)\n",
        saError, oh_lookup_error(saError)
    ));

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiResourcePowerStateSet (
    SAHPI_IN SaHpiSessionIdT       SessionId,
    SAHPI_IN SaHpiResourceIdT      ResourceId,
    SAHPI_IN SaHpiPowerStateT      State
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiResourcePowerStateSet() called with:\n"
        _PREFIX "        SessionId =      %d\n"
        _PREFIX "        ResourceId =     0x%x (%d)\n"
        _PREFIX "        ResetAction =    %s\n",
        (int)SessionId,
        (unsigned int)ResourceId, (int)ResourceId,
        oh_lookup_powerstate(State)
    ));

    saError = saHpiResourcePowerStateSet (SessionId, ResourceId, State);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiResourcePowerStateSet() returned with:\n"
        _PREFIX "        status =         %d (%s)\n",
        saError, oh_lookup_error(saError)
    ));

    return saError;
}

/*****************************************************************************/
SaErrorT SAHPI_API _saHpiSensorReadingGet (
    SAHPI_IN    SaHpiSessionIdT       SessionId,
    SAHPI_IN    SaHpiResourceIdT      ResourceId,
    SAHPI_IN    SaHpiSensorNumT       SensorNum,
    SAHPI_INOUT SaHpiSensorReadingT   *Reading,
    SAHPI_INOUT SaHpiEventStateT      *EventState
)
{
    SaErrorT saError;
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "saHpiSensorReadingGet() called with:\n"
        _PREFIX "        SessionId =      %d\n"
        _PREFIX "        ResourceId =     0x%x (%d)\n"
        _PREFIX "        SensorNum =      0x%x (%d)\n",
        (int)SessionId,
        (unsigned int)ResourceId, (int)ResourceId,
        (int)SensorNum, (int)SensorNum
    ));

    saError = saHpiSensorReadingGet (SessionId, ResourceId,
                                     SensorNum, Reading, EventState);
    
    CL_CM_HPI_DEBUG_PRINT((
        _PREFIX "... saHpiSensorReadingGet() returned with:\n"
        _PREFIX "        status =         %d (%s)\n"
        _PREFIX "        EventState =     0x%x\n"
        _PREFIX "        Reading =        <PRINTING NOT YET SUPPORTED>\n",
        saError, *EventState, oh_lookup_error(saError)
    ));
    
    return saError;
}

#endif /* CL_CM_HPI_CALL_DEBUG */

