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
 * ModuleName  : cm
 * File        : clCmFruHotSwapSm.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file implements the action handlers for the Hotswap state Transitions
 * for a FULL hotswappable capable FRU. The Handlers should not be confused
 * with the Transition handlers, these handlers are invoked after the
 * FRU has moved from the previous hotswap state to the present hotswap state.
 *
 *
 *****************************************************************************/

#include <stdio.h>

#include <SaHpi.h>
#include <oh_utils.h>

#include <clDebugApi.h>
#include <include/clCmDefs.h>
#include <clCmApi.h>

#include "clCmHpiDebug.h"
#include "clCmUtils.h"

#define AREA_FRU    "FRU"   /* log area */

/* NOTE: This value is the no of hotswap states in SaHpiHsStateT enumeration
 *  (refer to SaHpi.h )*/
#define SAHPI_NUM_HS_STATES 5

/* Chassis manager context information */
extern ClCmContextT gClCmChassisMgrContext;

typedef ClRcT (*ClCmFruStateTransitionHandlerT)(SaHpiRptEntryT *);

static ClRcT extractionPendingFromActive ( SaHpiRptEntryT * );
static ClRcT notPresentFromActive ( SaHpiRptEntryT * );
static ClRcT inactiveFromExtractionPending ( SaHpiRptEntryT * );
static ClRcT notPresentFromExtractionPending ( SaHpiRptEntryT * );
static ClRcT activeFromExtractionPending( SaHpiRptEntryT * );
static ClRcT insertionPendingFromInactive ( SaHpiRptEntryT * );
static ClRcT notPresentFromInactive ( SaHpiRptEntryT * );
static ClRcT insertionPendingFromNotPresent ( SaHpiRptEntryT * );
static ClRcT notPresentFromInsertionPending( SaHpiRptEntryT * );
static ClRcT activeFromInsertionPending ( SaHpiRptEntryT * );
static ClRcT inactiveFromInsertionPending ( SaHpiRptEntryT * );


/*
 * This is not a state transition handler table; these callbacks are called as
 * an after effect of the FRU moving in to the hotswap state.
 */

static ClCmFruStateTransitionHandlerT
                     /* presState */          /* prevState */
clCmFruStateCallbacks[SAHPI_NUM_HS_STATES][SAHPI_NUM_HS_STATES] =
    
{
/*          INACTIVE INSERTION_PENDING  ACTIVE EXTRACTION_PENDING  NOT_PRESENT  */
/*          ----------------------------------------------------------------------------*/
/*INACTIVE*/{NULL,inactiveFromInsertionPending,NULL,inactiveFromExtractionPending,NULL},
/*IP      */{insertionPendingFromInactive,NULL,NULL,NULL,insertionPendingFromNotPresent},
/*ACTIVE  */{NULL,activeFromInsertionPending,NULL,activeFromExtractionPending,NULL},
/*EP      */{NULL,NULL,extractionPendingFromActive,NULL,NULL},
/*NP      */{notPresentFromInactive,notPresentFromInsertionPending,notPresentFromActive,
             notPresentFromExtractionPending,NULL}
};


/*
 * Wrapper to invoke the state handlers.
 */
ClRcT clCmHandleFruHotSwapEvent( SaHpiHsStateT prevState,
                                 SaHpiHsStateT presState,
                                 SaHpiRptEntryT *pRptEntry )
{
    ClRcT rc = CL_OK;
    char *pEpathStr;
    
    /* Validate the states */
    if( prevState > SAHPI_HS_STATE_NOT_PRESENT ||
            presState > SAHPI_HS_STATE_NOT_PRESENT )
    {
        clLog(CL_LOG_ERROR, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
            "Invalid states passed to HS state machine");
        return CL_CM_ERROR(CL_ERR_INVALID_PARAMETER);
    }

    /* Invoke the state handler indexing by the previous and the
     * present states
     */
    pEpathStr = clCmEpath2String(&pRptEntry->ResourceEntity);

    if (clCmFruStateCallbacks[presState][prevState] != NULL )
    {
        rc = (*clCmFruStateCallbacks[presState][prevState])( pRptEntry);
        if (rc!=CL_OK)
        {
            clLog(CL_LOG_ERROR, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
                "HS state transition handler failed, error [0x%x]", rc);
        }
    }
    else
    {
        clLog(CL_LOG_WARNING, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
            "No handler for this HS state transition");
    }
    return CL_OK;
}

/* For Resources Having Full Hot Swap State M/c*/
static ClRcT clCmBladeInsertionHandling(SaHpiRptEntryT * pRptEntry)
{
    SaErrorT error = SA_OK;
    ClRcT rc=CL_OK;
    SaHpiTimeoutT       autoExtractionTimeOut;

    /*
     * Attempt to verify and correct the auto extraction timeout for the
     * newly inserted resource. The typical original value is 0, which triggers
     * an immediate execution of the FRU's default insertion policy (typically
     * a full power on). Instead, we set it a few seconds.
     * This gives us enough time to call the cancellation of the default
     * policy if needed (we don't do this yet, but will when an intelligent
     * blade admission policy is added), but in the same time it is low enough
     * that it does not make a visible difference for FRUs that are beyond
     * ASP's direct interest.
     */

    clLog(CL_LOG_NOTICE, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
          "Blade insertion requested for entity [%s] (resourceId [0x%x])",
           clCmEpath2String(&pRptEntry->ResourceEntity),
           (unsigned int) pRptEntry->ResourceId);
        
    /* Check the current auto-extract time first */
    if ((error = _saHpiAutoExtractTimeoutGet(
                        gClCmChassisMgrContext.platformInfo.hpiSessionId,
                        pRptEntry->ResourceId, 
                        &autoExtractionTimeOut
                        )) != SA_OK)
    {
        clLog(CL_LOG_WARNING, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
            "Could not get auto-extract timeout value for resource [0x%x] (%s); "
            "proceeding without check",
             (unsigned int)pRptEntry->ResourceId,
             oh_lookup_error(error));
    }
    else
    {
        /* Adjust timeout value if needed */
        clLog(CL_LOG_DEBUG, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
            "Current auto-extract timeout for [0x%x] is [%lld] ms",
             pRptEntry->ResourceId, autoExtractionTimeOut/1000L);

        if (autoExtractionTimeOut == SAHPI_TIMEOUT_BLOCK ||
            autoExtractionTimeOut != CL_CM_MIN_AUTOEXTRACT_TIMEOUT)
        {
            clLog(CL_LOG_DEBUG, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
                "Need to adjust auto-insert timeout to [%lld.%09lld] seconds",
                  (CL_CM_MIN_AUTOEXTRACT_TIMEOUT/1000000000L),
                  (CL_CM_MIN_AUTOEXTRACT_TIMEOUT%1000000000L));

            if (pRptEntry->HotSwapCapabilities & 
                    SAHPI_HS_CAPABILITY_AUTOEXTRACT_READ_ONLY)
            {
                clLog(CL_LOG_WARNING, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
                            "Cannot set read-only auto-extract timeout; "
                            "proceeding...");
            }
            else if ((error = _saHpiAutoExtractTimeoutSet(
                    gClCmChassisMgrContext.platformInfo.hpiSessionId,
                    pRptEntry->ResourceId,
                    CL_CM_MIN_AUTOEXTRACT_TIMEOUT
                    ))!= SA_OK )
            {
                clLog(CL_LOG_WARNING, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
                    "Setting auto-extract timeout for resource [0x%x] failed (%s); "
                    "proceeding nevertheless...",
                    pRptEntry->ResourceId,
                    oh_lookup_error(error));
            }
            else
            {
                clLog(CL_LOG_INFO, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
                    "Auto-extract timeout successfully adjusted for resource [0x%x]",
                    pRptEntry->ResourceId);
            }
        }
    }
    

#if 0 /* we should not need to mack with power and reset state; the shelf
       * manager should take care of allowing the blade
       */    
    if((pRptEntry->ResourceCapabilities & SAHPI_CAPABILITY_POWER)
        == SAHPI_CAPABILITY_POWER)
    {

        error = _saHpiResourcePowerStateSet(
              gClCmChassisMgrContext.platformInfo.hpiSessionId,
              pRptEntry->ResourceId,
              SAHPI_POWER_ON);
    
        if(error != SA_OK )
        {
            clLog(CL_LOG_ERROR, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
                "saHpiResourcePowerStateSet Failed :%s",
                oh_lookup_error(error));
            return CL_OK ;
        }
        clLog(CL_LOG_DEBUG, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
            "saHpiResourcePowerStateSet POWER ON succeeded");
    }
    else
    {
        clLog(CL_LOG_DEBUG, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
            "No Power Capability");
    }

    if((pRptEntry->ResourceCapabilities & SAHPI_CAPABILITY_RESET)
        == SAHPI_CAPABILITY_RESET)
    {

        error = _saHpiResourceResetStateSet(
              gClCmChassisMgrContext.platformInfo.hpiSessionId,
              pRptEntry->ResourceId,
              SAHPI_RESET_DEASSERT);
    
        if(error != SA_OK )
        {
            clLog(CL_LOG_ERROR, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
                "saHpiResourceResetStateSet DEASSERT failed: %s",
                 oh_lookup_error(error));
            return CL_OK ;
        }
        clLog(CL_LOG_DEBUG, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
            "saHpiResourceResetStateSet DEASSERT succeeded");
    }
    else
    {
        clLog(CL_LOG_DEBUG, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
            "No reset capability");
    }
#endif

    /* Inform the CPM-G */
    rc = clCmHotSwapCpmUpdate(pRptEntry, CL_CM_BLADE_REQ_INSERTION);
    if(rc!= CL_OK )
    {
        clLog(CL_LOG_ERROR, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
            "Failed to notify AMF about blade insertion, error [0x%x]", rc);
    }
    return CL_OK;
}


static ClRcT extractionPendingFromActive ( SaHpiRptEntryT * pRptEntry )
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
        "Active -> ExtractionPending: calling extraction request handler");

    /* Inform CPM about the FRU requesting to be extracted */
    rc=clCmHotSwapCpmUpdate(pRptEntry, CL_CM_BLADE_REQ_EXTRACTION);
    if (rc!= CL_OK )
    {
        clLog(CL_LOG_ERROR, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
            "Extraction request handler failed, error [0x%x]", rc);
    }
    return CL_OK;
}


static ClRcT notPresentFromActive ( SaHpiRptEntryT * pRptEntry )
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
        "Active -> NotPresent: calling surpise extraction handler");

    /* Surprise Extraction */
    rc= clCmHotSwapCpmUpdate(pRptEntry, CL_CM_BLADE_SURPRISE_EXTRACTION);
    if(CL_OK != rc)
    {
        clLog(CL_LOG_ERROR, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
            "Surprise extraction handler failed, error [0x%x]", rc);
    }
    return rc;
}


static ClRcT inactiveFromExtractionPending ( SaHpiRptEntryT * pRptEntry )
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
        "ExtractionPending -> Inactive: no-op");
    return rc;
}


static ClRcT notPresentFromExtractionPending ( SaHpiRptEntryT * pRptEntry )
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
        "ExtractionPending -> NotPresent: calling surpise extraction handler");

    /* Surprise Extraction */
    rc= clCmHotSwapCpmUpdate(pRptEntry, CL_CM_BLADE_SURPRISE_EXTRACTION);
    if(CL_OK != rc)
    {
        clLog(CL_LOG_ERROR, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
            "Surprise extraction handler failed, error [0x%x]", rc);
    }
    return rc;
}


static ClRcT activeFromExtractionPending( SaHpiRptEntryT * pRptEntry )
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
        "ExtractionPending -> Active: no-op");
    return rc;
}


static ClRcT insertionPendingFromInactive ( SaHpiRptEntryT * pRptEntry )
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
        "Inactive -> InsertionPending: calling blade insertion handler");
    rc = clCmBladeInsertionHandling(pRptEntry);
    return rc;
}


static ClRcT notPresentFromInactive ( SaHpiRptEntryT * pRptEntry )
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
        "Inactive -> NotPresent: no-op");
    return rc;
}


static ClRcT insertionPendingFromNotPresent ( SaHpiRptEntryT * pRptEntry )
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
        "NotPresent -> InsertionPending: calling blade insertion handler");
    rc = clCmBladeInsertionHandling(pRptEntry);
    return rc;
}


static ClRcT notPresentFromInsertionPending( SaHpiRptEntryT * pRptEntry )
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
        "InsertionPending -> NotPresent: no-op");
    return rc;
}


static ClRcT activeFromInsertionPending ( SaHpiRptEntryT * pRptEntry )
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
        "InsertionPending -> Active: no-op");
    return rc;
}


static ClRcT inactiveFromInsertionPending ( SaHpiRptEntryT * pRptEntry )
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, AREA_FRU, CL_LOG_CONTEXT_UNSPECIFIED,
        "InsertionPending -> Inactive: no-op");
    return rc;
}
