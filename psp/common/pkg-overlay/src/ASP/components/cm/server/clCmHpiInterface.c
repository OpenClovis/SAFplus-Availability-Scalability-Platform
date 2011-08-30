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
 * File        : clCmHpiInterface.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the interface implementation of the chassis manager 
 * with HPI.                                                              
 **************************************************************************/

#include <string.h>
#include <unistd.h>

#include <SaHpi.h>
#include <oh_utils.h>       /* From OpenHPI package */

#include <clCommon.h>
#include <clDebugApi.h>
#include <clCmApi.h>
#include <clAlarmDefinitions.h>
#include <include/clCmDefs.h>
#include <clOsalApi.h>
#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clLogApi.h>
#include <clCpmExtApi.h>
#include <include/clCmDefs.h>

#include <include/clCmDefs.h>
#include "clCmHpiDebug.h"
#include "clCmUtils.h"

#define AREA_HPI    "HPI"
#define CTX_INI     "INI"   /* Initialization context */
#define CTX_EVT     "EVT"   /* HPI event handler context */
#define CTX_DIS     "DIS"   /* discovery context */
#define CTX_REQ     "REQ"   /* client request context */

static ClRcT DiscoverPlatform ( SaHpiSessionIdT  session_id );
static ClRcT hpiEventProcessingLoop  ( void *ptr );
static ClRcT openHpiSession ( SaHpiSessionIdT *session_id,
                              SaHpiDomainIdT domain_id );
static ClRcT subscribeToPlatformEvents ( SaHpiSessionIdT *sessionid, ClOsalTaskIdT *);
static ClRcT resourceHotSwapEventProcess(SaHpiSessionIdT sessionid, 
                            SaHpiEventT *pEvent,
                            SaHpiRptEntryT * );
static ClRcT clCmSensorEventProcess( SaHpiSessionIdT sessionid,
                                     SaHpiEventT * pEvent, 
                                     SaHpiRptEntryT  *rptentry,
                                     SaHpiRdrT *pRdr);

/* Chassis manager context information */
extern ClCmContextT gClCmChassisMgrContext;


/* Called by the Chassis manager to initialize the HPI Interface */
ClRcT clCmHpiInterfaceInitialize(void )
{

    ClRcT rc = CL_OK;
    SaHpiVersionT hpiVersion = 0x0 ; 
    ClInt16T button;

    /* Try to find whether the HPI environment is set,if not freak out */

    /* ZSOLT: This is needed only when working with libopenhpi */
    if(getenv("OPENHPI_CONF") == NULL ){
        clLog(CL_LOG_CRITICAL, AREA_HPI, CTX_INI,
            "OPENHPI_CONF environment variable is not set. Please set it");
        return CL_CM_ERROR ( CL_ERR_INVALID_PARAMETER );
    }

    hpiVersion = _saHpiVersionGet();

    /* Negotiate the Version of HPI */
    if( hpiVersion != SAHPI_INTERFACE_VERSION )
    {
        clLog(CL_LOG_CRITICAL, AREA_HPI, CTX_INI,
              "HPI version mismatch between HPI library version [%#x] and CM version [%#x]",
              hpiVersion, SAHPI_INTERFACE_VERSION);
        return CL_CM_ERROR(CL_ERR_VERSION_MISMATCH);
    }

    clLog(CL_LOG_INFO, AREA_HPI, CTX_INI,
        "HPI Version found: [%c.0%d.0%d]",
        (unsigned char)(hpiVersion>>16)==0x01?'A':'B',
        (ClUint8T)(hpiVersion>>8),
        (ClUint8T)(hpiVersion));
         
    /* Open the Initial session to discover the hardware in the platform*/
    if((rc = openHpiSession ( 
                    &gClCmChassisMgrContext.platformInfo.hpiSessionId,
                    SAHPI_UNSPECIFIED_DOMAIN_ID
                    ))!= CL_OK ){
        clLog(CL_LOG_ERROR, AREA_HPI, CTX_INI,
            "Could not open HPI session, error [0x%x]", rc);
        return rc;
    }

    /* Discover the Platform Hardware in the System */ 
    button = CL_TRUE;
    clCmDisplayProgressBar (&button);

    if((rc = DiscoverPlatform(
                    gClCmChassisMgrContext.platformInfo.hpiSessionId 
                    ))!= CL_OK ){
        button = CL_FALSE;
        clLog(CL_LOG_CRITICAL, AREA_HPI, CTX_INI,
            "DiscoverPlatform failed, error [0x%x]", rc);
        _saHpiSessionClose(gClCmChassisMgrContext.platformInfo.hpiSessionId);
        return rc;
    }
    button = CL_FALSE;

    /* Starts the Event listener thread and listens to Platform Events*/
    clLog(CL_LOG_DEBUG, AREA_HPI, CTX_INI,
        "Launching HPI event receiver loop thread");
    if((rc = subscribeToPlatformEvents( 
                    &gClCmChassisMgrContext.platformInfo.hpiSessionId,
                    &(gClCmChassisMgrContext.eventListenerThread)
                    ))!= CL_OK ){
        clLog(CL_LOG_CRITICAL, AREA_HPI, CTX_INI,
            "Failed to subscribe to platform events, error [0x%x]", rc);
        _saHpiSessionClose(gClCmChassisMgrContext.platformInfo.hpiSessionId);
        return rc;
    }
    return rc;
}


/* Called by the Chassis manager to finalize the HPI Interface */
ClRcT 
clCmHpiInterfaceFinalize(void )
{
    ClRcT rc = CL_OK;
    ClTimerTimeOutT delay = {1 , 0};
    
    gClCmChassisMgrContext.platformInfo.stopEventProcessing = CL_TRUE;
    (void)clOsalTaskDelay(delay); /* wait a bit before killing the thread */

    clOsalTaskDelete(gClCmChassisMgrContext.eventListenerThread);
    _saHpiSessionClose(gClCmChassisMgrContext.platformInfo.hpiSessionId);
    sleep(1);

    return rc;
}


/*Called by Chassis manager to discover the resources in the Platform, This
 * function is called 
 *                    i)  During Startup of the Chassis manager . 
 *                    ii) Up on User requests from NB Interface. 
 *                    iii)Up on any significant events like FRU Arrival,
 *                        Departure etc. ZSOLT: really???
 */
static ClRcT DiscoverPlatform ( SaHpiSessionIdT  session_id ) 
{
    SaErrorT         error = SA_OK ;
    SaHpiDrtEntryT   drt_entry;
    ClRcT            rc = CL_OK;

    gClCmChassisMgrContext.platformInfo.discoveryInProgress = CL_TRUE;
    memset((void*)&drt_entry, 0,sizeof(drt_entry));

    clLog(CL_LOG_INFO, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
        "Platform discovery started");

    /* discover the domain and all the FRU's in the system */ 
    if((error = _saHpiDiscover( session_id ))!= SA_OK ){
        clLog(CL_LOG_CRITICAL, AREA_HPI, CTX_DIS,
            "saHpiDiscover failed: %s", oh_lookup_error(error));
        gClCmChassisMgrContext.platformInfo.discoveryInProgress = CL_FALSE; 
        return CL_CM_ERROR(CL_ERR_CM_HPI_ERROR);
    }

    clLog(CL_LOG_INFO, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
        "HPI discovery phase complete, starting inventory walk-through");

    /* FIXME: API is not implemented by the plugin ABI uncomment after the API is
     * implemented */ 
#if 0 

    SaHpiEntryIdT    current, next;

    /* list out the domains in the DRT table */ 
    current = SAHPI_FIRST_ENTRY;
    while( current != SAHPI_LAST_ENTRY ){
        if((error = _saHpiDrtEntryGet( session_id, 
                        current, 
                        &next, 
                        &drt_entry ))!= SA_OK ){
            /* FIXME proper error message comes here */
            return CL_CM_ERROR(CL_ERR_CM_HPI_ERROR);
        }
        /* for each domain entry walk through the RPT and list the RDRs */ 
        scan_domain( &drt_entry );
        current = next;
    }
#endif 

    drt_entry.DomainId = SAHPI_UNSPECIFIED_DOMAIN_ID;
    if ((rc = WalkThroughDomain (&drt_entry))!= CL_OK)
    {
        
        clLog(CL_LOG_ERROR, AREA_HPI, CTX_DIS,
            "WalkThroughDomain failed, error [0x%x]", rc);
    }
    gClCmChassisMgrContext.platformInfo.discoveryInProgress = CL_FALSE; 

    clLog(CL_LOG_INFO, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
        "Inventory walk-through completed");

    return CL_OK;
}


/*
 * Walk function to walk through the resource and find the RDR's for that
 * resource
 */
ClRcT WalkThroughResource(SaHpiRptEntryT *rpt_entry,
                          SaHpiSessionIdT session_id)
{
    SaHpiResourceIdT    resourceid;
    SaHpiEntryIdT       entryid; 
    SaHpiEntryIdT       nextentryid; 
    SaHpiRdrT            rdr;
    SaErrorT            error = SA_OK ;

    /* Nothing to do if resource does not have RDR capability */
    if ((rpt_entry->ResourceCapabilities & SAHPI_CAPABILITY_RDR)
            != SAHPI_CAPABILITY_RDR)
    {
        return CL_OK;
    }
    
    /* walk through the list of RDRs 
     * for this RPT entry and display them
     */
    resourceid = rpt_entry->ResourceId;

    memset(&rdr,0,sizeof(SaHpiRdrT));

    entryid = SAHPI_FIRST_ENTRY;
    while(entryid != SAHPI_LAST_ENTRY )
    {
        if ((error = _saHpiRdrGet(session_id, resourceid, entryid,
                                  &nextentryid, &rdr ))!= SA_OK)
        {
             clLog(CL_LOG_ERROR, AREA_HPI, CTX_DIS,
                "saHpiRdrGet failed: %s", oh_lookup_error(error));
             return CL_CM_ERROR(CL_ERR_CM_HPI_ERROR);
        }

        switch ( rdr.RdrType )
        {
        case SAHPI_SENSOR_RDR : 
            gClCmChassisMgrContext.platformInfo.no_of_sensors++;
            break;

        case SAHPI_CTRL_RDR:    
            break;

        case SAHPI_INVENTORY_RDR:
            if ((rpt_entry->ResourceCapabilities & SAHPI_CAPABILITY_INVENTORY_DATA)
                == SAHPI_CAPABILITY_INVENTORY_DATA)
            {
                gClCmChassisMgrContext.platformInfo.no_of_inventories++;
            }
            break;

        case SAHPI_WATCHDOG_RDR:
            break;

        case SAHPI_ANNUNCIATOR_RDR:
            break;

        default: 
            clLog(CL_LOG_ERROR, AREA_HPI, CTX_DIS,
                "Unknown resource type detected");
            break;
        }
        entryid = nextentryid;
        memset(&rdr,'\0',sizeof(SaHpiRdrT));

    } /* end while */
    return CL_OK;
}


/* Walk through the RPT table and list all the entries in the RPT Table */ 
ClRcT WalkThroughDomain( SaHpiDrtEntryT   *drt_entry )
{
    SaHpiEntryIdT       current = SAHPI_FIRST_ENTRY, next;
    SaHpiRptEntryT      rpt_entry;
    SaErrorT            error = SA_OK; 
    SaHpiSessionIdT     session_id = 0x0 ; 
    ClRcT               rc = CL_OK;
    SaHpiTimeoutT       autoExtractionTimeOut;
    SaHpiTimeoutT       autoInsertionTimeOut;

    /* Open the session for the domain id and scan through the RPT Table */ 
    if( openHpiSession( &session_id, drt_entry->DomainId )!= CL_OK )
    {
        clLog(CL_LOG_ERROR, AREA_HPI, CTX_DIS,
            "Unable to open new HPI session for resource walk-through");
        return CL_CM_ERROR(CL_ERR_CM_HPI_ERROR);
    }

    /* Verify that the preset autoinsertion timeout is large enough. */
    if ((error = _saHpiAutoInsertTimeoutGet(
                    gClCmChassisMgrContext.platformInfo.hpiSessionId,
                    &autoInsertionTimeOut)) != SA_OK)
    {
        clLog(CL_LOG_WARNING, AREA_HPI, CTX_DIS,
            "Could not obtain auto-insert timeout value; "
            "proceeding without check");
    }
    else
    {
        clLog(CL_LOG_DEBUG, AREA_HPI, CTX_DIS,
            "Current auto-insert timeout is set to [%lld] ms",
             autoInsertionTimeOut/1000L);
        /*
         * We want to set this if it is too low (and if it can be altered.
         * If it is set to block, also change it to our preferred value.
         */
        if (autoInsertionTimeOut == SAHPI_TIMEOUT_BLOCK ||
            autoInsertionTimeOut < CL_CM_MIN_AUTOINSERT_TIMEOUT)
        {
            clLog(CL_LOG_DEBUG, AREA_HPI, CTX_DIS,
                "Need to adjust auto-insert timeout to [%lld.%09lld] seconds",
                  (CL_CM_MIN_AUTOINSERT_TIMEOUT/1000000000L),
                  (CL_CM_MIN_AUTOINSERT_TIMEOUT%1000000000L));

            if ((error = _saHpiAutoInsertTimeoutSet(
                    gClCmChassisMgrContext.platformInfo.hpiSessionId, 
                    CL_CM_MIN_AUTOINSERT_TIMEOUT
                    ))!= SA_OK )
            {
                clLog(CL_LOG_WARNING, AREA_HPI, CTX_DIS,
                    "Could not set auto-insert timeout value (%s); "
                    "proceeding nevertheless...", oh_lookup_error(error));
            }
            else
            {
                clLog(CL_LOG_INFO, AREA_HPI, CTX_DIS,
                    "Chassis auto-insert timeout adjusted successfully");
            }
        }
    }

    /* RPT Table is formed now scan each RPT rpt_entry and display the
     * capabilities of the Entities */ 
    memset(&rpt_entry,'\0',sizeof(SaHpiRptEntryT)); 

    current = SAHPI_FIRST_ENTRY;
    while (current != SAHPI_LAST_ENTRY)
    {
        if ((error = _saHpiRptEntryGet(session_id, current, &next, &rpt_entry))
                     != SA_OK)
        {
            clLog(CL_LOG_ERROR, AREA_HPI, CTX_DIS,
                "saHpiRPTEntryGet failed: %s", oh_lookup_error(error));
            _saHpiSessionClose(session_id);
            return CL_CM_ERROR(CL_ERR_CM_HPI_ERROR);
        }

        clLog(CL_LOG_INFO, AREA_HPI, CTX_DIS,
            "HPI resource id [0x%x] is [%s]",
            rpt_entry.ResourceId,
            clCmEpath2String(&rpt_entry.ResourceEntity));
        /* find out the FRUs */ 
        if ((rpt_entry.ResourceCapabilities & SAHPI_CAPABILITY_FRU ) &&
            (rpt_entry.ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP))
        {
            /* Check the auto-extract time first */
            if ((error = _saHpiAutoExtractTimeoutGet(
                                gClCmChassisMgrContext.platformInfo.hpiSessionId,
                                rpt_entry.ResourceId, 
                                &autoExtractionTimeOut
                                )) != SA_OK)
            {
                clLog(CL_LOG_WARNING, AREA_HPI, CTX_DIS,
                    "Could not get auto-extract timeout value for resource [0x%x] (%s); "
                    "proceeding without check",
                     (unsigned int)rpt_entry.ResourceId,
                     oh_lookup_error(error));
            }
            else
            {
                /* Adjust timeout value if needed */
                clLog(CL_LOG_DEBUG, AREA_HPI, CTX_DIS,
                    "Current auto-extract timeout for [0x%x] is %lld [ms]",
                    rpt_entry.ResourceId, autoExtractionTimeOut/1000L);

                if (autoExtractionTimeOut == SAHPI_TIMEOUT_BLOCK ||
                    autoExtractionTimeOut != CL_CM_MIN_AUTOEXTRACT_TIMEOUT)
                {
                    clLog(CL_LOG_DEBUG, AREA_HPI, CTX_DIS,
                        "Need to adjust auto-extract timeout to "
                         "[%lld.%09lld] seconds for resource [0x%x]",
                          (CL_CM_MIN_AUTOEXTRACT_TIMEOUT/1000000000L),
                          (CL_CM_MIN_AUTOEXTRACT_TIMEOUT%1000000000L),
                          rpt_entry.ResourceId);

                    if (rpt_entry.HotSwapCapabilities & 
                            SAHPI_HS_CAPABILITY_AUTOEXTRACT_READ_ONLY)
                    {
                        clLog(CL_LOG_WARNING, AREA_HPI, CTX_DIS,
                            "Cannot set read-only auto-extract timeout; "
                            "proceeding...");
                    }
                    else if ((error = _saHpiAutoExtractTimeoutSet(
                            gClCmChassisMgrContext.platformInfo.hpiSessionId,
                            rpt_entry.ResourceId,
                            CL_CM_MIN_AUTOEXTRACT_TIMEOUT
                            ))!= SA_OK )
                    {
                        clLog(CL_LOG_WARNING, AREA_HPI, CTX_DIS,
                            "Setting auto-extract timeout for resource [0x%x] failed (%s); "
                            "proceeding nevertheless...",
                            rpt_entry.ResourceId,
                            oh_lookup_error(error));
                    }
                    else
                    {
                        clLog(CL_LOG_INFO, AREA_HPI, CTX_DIS,
                            "Auto-extract timeout successfully adjusted for resource [0x%x]",
                            rpt_entry.ResourceId);
                    }
                }
            }

            /* Counting # of FRUs */
            gClCmChassisMgrContext.platformInfo.no_of_frus++; 
        }

        /* display Resource data records assosciated with the resource */
        if ((rc = WalkThroughResource(&rpt_entry, session_id))!= CL_OK)
        {
            clLog(CL_LOG_ERROR, AREA_HPI, CTX_DIS,
                "WalkThroughResource failed, error [0x%x]", rc);
            _saHpiSessionClose(session_id);
            return rc;
        }

        current = next;
        memset(&rpt_entry,'\0',sizeof(SaHpiRptEntryT)); 

    }
    _saHpiSessionClose(session_id);
    return CL_OK;
}


/* Called by the Chassis manager to open a HPI session with the Default Domain
 * Id (defined in SaHpi.h header ). 
 */
static ClRcT 
openHpiSession( SaHpiSessionIdT *session_id, SaHpiDomainIdT  domain_id )
{
    SaErrorT       error = SA_OK;
   
    if ((error = _saHpiSessionOpen(domain_id, session_id, NULL))!= SA_OK)
    {
        clLog(CL_LOG_CRITICAL, AREA_HPI, CTX_INI,
            "saHpiSessionOpen failed: %s", oh_lookup_error(error));
        return CL_CM_ERROR(CL_ERR_CM_HPI_ERROR);
    }
    return CL_OK;
}


/*****************************************************************************/
/*                            HPI EVENT HANDLING                                  */
/*****************************************************************************/

/* Subscribe to the platform events from HPI 
    Platform Events like :
        i) FRU Arrival, Departure etc.
        ii) Sensor asserts and Deasserts etc.
*/
static ClRcT
subscribeToPlatformEvents(SaHpiSessionIdT *sessionid,
                          ClOsalTaskIdT *pEventListenerThread)
{
    ClRcT rc = CL_OK;

    /* Create the Event Listener Thread and process the Events */
    if ((rc = clOsalTaskCreateAttached(
                    "Event listener thread",
                    CL_OSAL_SCHED_OTHER,
                    CL_OSAL_THREAD_PRI_NOT_APPLICABLE, /*Priority*/
                    8*1024*1024, /* Stack Size - Fix to Bug 5266 */
                                 /* The 8MB comes from OpenHPI sample code */
                    (void*(*)(void *))hpiEventProcessingLoop,
                    (void *)sessionid,
                    pEventListenerThread
                    ))!= CL_OK )
    {
        clLog(CL_LOG_CRITICAL, AREA_HPI, CTX_INI,
            "Failed to allocate trhead, error [0x%x]", rc);
    }

    return rc;
}


static char *
eventEpathToString(SaHpiEventT *event, SaHpiEntityPathT *epath)
{
    guint id;
    static oh_big_textbuffer buffer;
    SaHpiEntityPathT ep;
    SaErrorT err = SA_OK;
    
    id = event->Source;
    if (epath) {
        err  = oh_decode_entitypath(epath, &buffer);
    } else {
        err = oh_entity_path_lookup(id, &ep);
        if (err) {
            clLog(CL_LOG_ERROR, AREA_HPI, CTX_EVT,
                "Could not determine entity path for resource id [0x%x]", id);
        } else {
            err  = oh_decode_entitypath(&ep, &buffer);
        }
    }
    if (err == SA_OK)
        return (char*)&buffer.Data;
    else
        return NULL;

}


static char *
eventDetailsToString(SaHpiEventT *event, SaHpiEntityPathT *epath)
{
    ClCharT str[SAHPI_MAX_TEXT_BUFFER_LENGTH], *str2;
    static oh_big_textbuffer buffer;
    SaHpiTextBufferT tmpbuffer;
    SaErrorT err = SA_OK;
   
    (void)oh_init_bigtext(&buffer);
    
    /* type */
    snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH,
             "\ntype:           %s",
             oh_lookup_eventtype(event->EventType));
    oh_append_bigtext(&buffer, str);
    
    /* severity */
    snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH,
             "\nseverity:       %s",
             oh_lookup_severity(event->Severity));
    oh_append_bigtext(&buffer, str);
    
    /* epath */
    str2 = eventEpathToString(event, epath);
    if (str2 != NULL)
    {
        snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH,
             "\nentity:         %s", str2);
        oh_append_bigtext(&buffer, str);
    }
    
    /* resource id */
    snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH,
             "\nresid:          %d (0x%x)",
             event->Source, event->Source);
    oh_append_bigtext(&buffer, str);
    
    switch (event->EventType)
    {
    case SAHPI_ET_SENSOR:

        snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH,
             "\nsensor-id:      %d (0x%x)",
             event->EventDataUnion.SensorEvent.SensorNum,
             event->EventDataUnion.SensorEvent.SensorNum);
        oh_append_bigtext(&buffer, str);

        snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH,
             "\nsensor-type:    %s",
             oh_lookup_sensortype(event->EventDataUnion.SensorEvent.SensorType));
        oh_append_bigtext(&buffer, str);

        snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH,
             "\nasserted:       %s",
             (event->EventDataUnion.SensorEvent.Assertion == SAHPI_TRUE) ?
             "TRUE" : "FALSE");
        oh_append_bigtext(&buffer, str);

        err = oh_decode_eventstate(event->EventDataUnion.SensorEvent.EventState,
                                   event->EventDataUnion.SensorEvent.EventCategory,
                                   &tmpbuffer);
        snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH,
             "\nstate:          %s",
             err ? "<unknown>" : (char*)tmpbuffer.Data);
        oh_append_bigtext(&buffer, str);
             
        break;
    case SAHPI_ET_HOTSWAP:
        break;
    default:
        break;
    }
    
    /* we do assume here that the data is an ASCII null-terminated string */
    return (char*)buffer.Data;
}
    
static ClRcT openHpiSessionReopen(SaHpiSessionIdT *sessionId)
{
    SaErrorT error = SA_OK;
    ClRcT rc = openHpiSession(sessionId, SAHPI_UNSPECIFIED_DOMAIN_ID);
    if(rc != CL_OK)
    {
        clLogError(AREA_HPI, "REOPEN", "Could not reopen hpi session. Error [%#x]", rc);
        return rc;
    }
    error = _saHpiSubscribe(*sessionId);
    if(error != SA_OK)
    {
        clLogError(AREA_HPI, "REOPEN", "saHpiSubscribe failed with error [%s]", oh_lookup_error(error));
        _saHpiSessionClose(*sessionId);
        *sessionId = 0;
        return CL_CM_ERROR(CL_ERR_CM_HPI_ERROR);
    }
    return CL_OK;
}

/* Event listener loop listens for the Events from the HPI */
static ClRcT hpiEventProcessingLoop(void *ptr)
{
#define SAHPI_EVENT_TIMEOUT (2 * 1000 * 1000 * 1000LL) /* 2 seconds*/
    SaHpiRptEntryT rptentry ; 
    SaHpiRdrT        rdr;
    SaHpiEventT    event;
    SaErrorT       error = SA_OK ;
    ClRcT            rc = CL_OK;
    SaHpiSessionIdT sessionid = 0x0;
    SaHpiEvtQueueStatusT     qstatus; 
    static int errcnt;

    memset(&rptentry, 0, sizeof(rptentry));
    memset(&rdr, 0, sizeof(rdr));
    memset(&event, 0, sizeof(event));
    sessionid = *(SaHpiSessionIdT*)ptr;

    /* set the execution object handle of Chassis manager */
    if((rc = clEoMyEoObjectSet(gClCmChassisMgrContext.executionObjHandle))
                               != CL_OK )
    {
        clLog(CL_LOG_CRITICAL, AREA_HPI, CTX_INI,
            "Unable to set execution object handle, error [0x%x]", rc);
        return rc;
    }


    /* call hpi event subscribe to subscribe for platform events */
    if( (error = _saHpiSubscribe( sessionid ))!= SA_OK  )
    {
        clLog(CL_LOG_CRITICAL, AREA_HPI, CTX_INI,
            "saHpiSubscribe failed: %s", oh_lookup_error(error));
            return CL_CM_ERROR(CL_ERR_CM_HPI_ERROR);
    }
    
    clLog(CL_LOG_INFO, AREA_HPI, CTX_INI,
        "Entering HPI event receive loop");
    
    while(! gClCmChassisMgrContext.platformInfo.stopEventProcessing )
    {
        /* Get the events from the Domain Event Table */
        error = _saHpiEventGet(sessionid, 
                               SAHPI_EVENT_TIMEOUT,
                               &event, 
                               &rdr,
                               &rptentry,
                               &qstatus);
        if(error != SA_OK)
        {
            static ClTimerTimeOutT delay = { .tsSec = 2, .tsMilliSec = 0 };
            if(error == SA_ERR_HPI_TIMEOUT)
                continue;
            /*
             * Try reopening the session some finite number of times
             * before giving up.
             */
            clLog(CL_LOG_CRITICAL, AREA_HPI, CTX_EVT,
                  "saHpiEventGet failed: %s", oh_lookup_error(error));
            _saHpiUnsubscribe(sessionid);
            if(++errcnt >= 4)
            {
                return CL_CM_ERROR(CL_ERR_CM_HPI_ERROR);
            }
            clOsalTaskDelay(delay);
            if( (rc = openHpiSessionReopen((SaHpiSessionIdT*)ptr)) != CL_OK)
            {
                return rc;
            }
            sessionid = *(SaHpiSessionIdT*)ptr;
            continue;
        }

        errcnt = 0;

        /* Check whether we lost any events due to queue filling */
        if( qstatus == SAHPI_EVT_QUEUE_OVERFLOW )
        {
            clLog(CL_LOG_WARNING, AREA_HPI, CTX_EVT,
                "HPI event queue overflow (some event may have been lost)");
        }

        /* only the active system contoller card supposed to generate events */
        /* (void)oh_fprint_event(stdout, &event, &(rdr.Entity), 12); */
        clLogMultiline(CL_LOG_DEBUG, AREA_HPI, CTX_EVT,
            "HPI event received:%s", eventDetailsToString(&event, &(rdr.Entity)));
            
        if (CL_TRUE != clCpmIsMaster())
        {
            clLog(CL_LOG_DEBUG, AREA_HPI, CTX_EVT,
                "Standby CM ignores event");
        }
        else
        {
            switch(event.EventType)
            {

            case  SAHPI_ET_RESOURCE:

                /*
                 * Handle Surprise Extraction on Radisys Chassis, which
                 * is observed as a MAJOR FAILURE on a resource
                 */

			    if ((event.EventDataUnion.ResourceEvent.ResourceEventType == SAHPI_RESE_RESOURCE_FAILURE)
					 && (event.Severity == SAHPI_MAJOR))
			    {
                    clLogMultiline(CL_LOG_NOTICE, AREA_HPI, CTX_EVT,
                        "MAJOR HPI resource failure reported on [%s]\n"
                        "Treating it as surprise extraction",
                        eventEpathToString(&event, &(rdr.Entity)));
                    
                    rc = clCmHotSwapCpmUpdate(&rptentry,
                                            CL_CM_BLADE_SURPRISE_EXTRACTION);
                    if (CL_OK != rc)
                    {       
	                    clLog(CL_LOG_NOTICE, AREA_HPI, CTX_EVT,
                            "Failed to report surprise extraction, error [0x%x]", rc);
                        /* Do not return as the info in cor needs to be updated*/
                    }
			    }
                continue;

            case  SAHPI_ET_SENSOR  :
                if ((rc = clCmSensorEventProcess(sessionid, &event, &rptentry,
                                                 &rdr)) != CL_OK)
                {
                    clLog(CL_LOG_ERROR, AREA_HPI, CTX_EVT,
                        "Sensor event processing failed, error [0x%x]", rc);
                }
                continue;

            case  SAHPI_ET_HOTSWAP :
                /* Process the Hotswap event */
                if ((rc = resourceHotSwapEventProcess(sessionid, &event,
                                                      &rptentry))!= CL_OK )
                {
                    clLog(CL_LOG_ERROR, AREA_HPI, CTX_EVT,
                        "Hotswap event processing failed, error [0x%x]", rc);
                }
                continue;

            case  SAHPI_ET_DOMAIN:
                clLog(CL_LOG_DEBUG, AREA_HPI, CTX_EVT, "Domain events ignored by CM");
                continue;

            case  SAHPI_ET_WATCHDOG:
                clLog(CL_LOG_DEBUG, AREA_HPI, CTX_EVT, "Watchdog events ignored by CM");
                continue;

            case  SAHPI_ET_OEM     :
                clLog(CL_LOG_DEBUG, AREA_HPI, CTX_EVT, "OEM events ignored by CM");
                continue;

            case  SAHPI_ET_USER    :
                clLog(CL_LOG_DEBUG, AREA_HPI, CTX_EVT, "User events ignored by CM");
                continue;

            case  SAHPI_ET_SENSOR_ENABLE_CHANGE:     
                clLog(CL_LOG_DEBUG, AREA_HPI, CTX_EVT,
                    "Sensor enable events ignored by CM");
                continue;

            default :
                clLog(CL_LOG_ERROR, AREA_HPI, CTX_EVT,
                    "Invalid event type [%d] received, ignoring", event.EventType);
                continue;

            }
        }
    } /*end while Loop */

    clLog(CL_LOG_INFO, AREA_HPI, CL_LOG_CONTEXT_UNSPECIFIED,
        "Leaving HPI event receive loop");
    
    _saHpiUnsubscribe( sessionid );
    return SA_OK;

#undef SAHPI_EVENT_TIMEOUT
}


/* Resource hotswap processing :
   -----------------------------
   Resources can follow either fullhotswap state machine or simplified hotswap
   state machine. FRUs which follow full hotswap state machine mostly will be
   running ASP instance on them and they have a physical slot in their entity
   paths, so the rest of the system needs to be notified about the node
   departure. FRUs which follow simplified hotswap model doesnt need any
   notification to the rest of the system since they will be FANS,
   POWERSUPPLIES we need to just update the management view of the Domain.

   This assumption is not true, as BCT blades also follow Simple Hot Swap
   Model.
 */
static ClRcT resourceHotSwapEventProcess( SaHpiSessionIdT sessionid,
                                          SaHpiEventT * pEvent, 
                                          SaHpiRptEntryT * pRptEntry) 
{ 
    ClRcT rc = CL_OK;
    ClCharT *epath = NULL;
    ClCmFruEventInfoT *pFruEventPayload;

    /* print the entity path in to the logfile and the previous and the
     *  present states */
    epath = clCmEpath2String(&pRptEntry->ResourceEntity);

    clLogMultiline(CL_LOG_NOTICE, AREA_HPI, CTX_EVT,
        "Hot-swap event received for resource [0x%x]:\n"
        "entity:            %s\n"
        "new-state:         %s\n"
        "previous-state:    %s",
        pEvent->Source,
        epath,
        oh_lookup_hsstate(pEvent->EventDataUnion.HotSwapEvent.HotSwapState),
        oh_lookup_hsstate(pEvent->
                            EventDataUnion.HotSwapEvent.PreviousHotSwapState));

    /* Publish FRU State Change Event to CM Event Channel */
    pFruEventPayload = clHeapAllocate(sizeof(ClCmFruEventInfoT));
    if (NULL == pFruEventPayload)
    {
        return CL_ERR_NULL_POINTER;
    }

    pFruEventPayload->fruId = pEvent->Source;
    pFruEventPayload->physicalSlot = clCmPhysicalSlotFromEntityPath(&(pRptEntry->ResourceEntity));
    pFruEventPayload->previousState = pEvent->EventDataUnion.HotSwapEvent.PreviousHotSwapState;
    pFruEventPayload->presentState = pEvent->EventDataUnion.HotSwapEvent.HotSwapState;

    rc = clCmPublishEvent(CM_FRU_STATE_TRANSITION_EVENT,
                          sizeof(ClCmFruEventInfoT),
                          pFruEventPayload);

    if (CL_OK != rc)
    {
        clLog(CL_LOG_CRITICAL, AREA_HPI, CTX_EVT,
              "Failed to publish FRU state transition event, error[0x%x]", rc);
    }

    clHeapFree(pFruEventPayload);

    /* Check the previous state of the FRU whether its a valid transition or
     *  Invalid Transition. If its a valid transition then query CPMG whether
     *  we can should allow the arrival or departure of the FRU in to the
     *  system. This query is an asynchronous query and the FRU waits in the
     *  EXTRACTION_PENDING state or INSERTION_PENDING state until the CPMG
     *  responds back to the Chassis manager query 
     */
    if((pRptEntry->ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP)
            ==  SAHPI_CAPABILITY_MANAGED_HOTSWAP)
    {

        rc = clCmHandleFruHotSwapEvent(
                pEvent->EventDataUnion.HotSwapEvent.PreviousHotSwapState,
                pEvent->EventDataUnion.HotSwapEvent.HotSwapState,
                pRptEntry);
    }
    else
    {
        /*
         * We cannot control the hotswap of the resource so we just update the
         * management view
         */
        rc = clCmSimpleHotSwapHandler(sessionid, pEvent, pRptEntry);
    }
    return rc;
}



/* 
   Description
   ------------
   Handles the response from the CPMG for the previous query made. 
 */
void 
clCmHPICpmResponseHandle(ClCpmCmMsgT * pCpm2CmMsg)
{
    SaErrorT saError;
    SaHpiRptEntryT rptEntry;

    clLogMultiline(CL_LOG_DEBUG, AREA_HPI, CTX_REQ,
        "Response from AMF arrived:\n"
        "message-type:      %d\n"
        "physical-slot:     %d\n"
        "resource-id:       %d (0x%x)\n"
        "response-type:     %d",
        pCpm2CmMsg->cmCpmMsg.cmCpmMsgType,
        pCpm2CmMsg->cmCpmMsg.physicalSlot,
        pCpm2CmMsg->cmCpmMsg.resourceId,
        pCpm2CmMsg->cmCpmMsg.resourceId,
        pCpm2CmMsg->cpmCmMsgType);
    
    saError = _saHpiRptEntryGetByResourceId(
            gClCmChassisMgrContext.
            platformInfo.hpiSessionId,
            pCpm2CmMsg->cmCpmMsg.resourceId,
            &rptEntry
            );
    if (SA_OK !=saError)
    {
        clLog(CL_LOG_WARNING,  AREA_HPI, CTX_REQ,
            "saHpiRptEntryGetByResourceId failed: %s",
            oh_lookup_error(saError));
        return ;
    }

    switch(pCpm2CmMsg->cmCpmMsg.cmCpmMsgType)
    {
        case CL_CM_BLADE_REQ_EXTRACTION:
        {
            /* No-op for now */
            break ;
        }
        case CL_CM_BLADE_REQ_INSERTION:
        {
            /* No-op for now */
            break;
        }
        case CL_CM_BLADE_SURPRISE_EXTRACTION:
        {
            /* No Handling from CM */
            break;

        }
        default :
        {
            clLog(CL_LOG_ERROR,  AREA_HPI, CTX_REQ,
                "Invalid message type [%d] from AMF",
                pCpm2CmMsg->cmCpmMsg.cmCpmMsgType);
        }
    }
    return ;
}


ClRcT clCmHotSwapCpmUpdate(SaHpiRptEntryT *pRptEntry,
                           ClCmCpmMsgTypeT cm2CpmMsgType)
{
    ClRcT rc = CL_OK;
    ClUint32T physicalSlot = CL_CM_INVALID_PHYSICAL_SLOT;
    ClCmCpmMsgT cm2CpmMsg;
    ClCharT *epath = NULL;
    
    epath = clCmEpath2String(&pRptEntry->ResourceEntity);
    if (NULL == epath)
        epath = "<could not decode>";

    clLog(CL_LOG_ALERT, AREA_HPI, CTX_EVT,
        "Hot-swap state change event received for [%s]", epath);
        
    physicalSlot = clCmPhysicalSlotFromEntityPath(&(pRptEntry->ResourceEntity));
    if (CL_CM_INVALID_PHYSICAL_SLOT != physicalSlot)
    {
        clLog(CL_LOG_INFO, AREA_HPI, CTX_EVT,
            "Entity path resolves to slot [%d], informing AMF", physicalSlot);
    }
    else 
    {
        clLog(CL_LOG_INFO, AREA_HPI, CTX_EVT,
            "Could not resolve entity path to a physical slot, ignoring surprise extraction");
        return CL_CM_ERROR( CL_ERR_CM_HPI_INVALID_PHY_SLOT);
    }

    cm2CpmMsg.physicalSlot=physicalSlot;
    cm2CpmMsg.cmCpmMsgType=cm2CpmMsgType;
    /**FIXME AMC handling, fill up the subslot sending 0 as of now */
    cm2CpmMsg.subSlot = 0;
    cm2CpmMsg.resourceId=pRptEntry->ResourceId;

    rc = clCpmHotSwapEventHandle(&cm2CpmMsg);
    if (rc!=CL_OK ) 
    {
        clLog(CL_LOG_ERROR, AREA_HPI, CTX_EVT,
            "Failed to inform AMF, error [0x%x]", rc);
    }
    return rc;
}
    


/*
 * Simply update the information in COR so that the management sees the
 * updated view of the domain.
 */
ClRcT 
clCmSimpleHotSwapHandler (SaHpiSessionIdT sessionid, 
                          SaHpiEventT * pEvent,
                          SaHpiRptEntryT  * pRptEntry)
{
    ClRcT rc = CL_OK;

    if (SAHPI_HS_STATE_ACTIVE ==
        pEvent->EventDataUnion.HotSwapEvent.HotSwapState) 
    {
        /* No Information needs to be sent to CPM-G*/
    }
    else if (SAHPI_HS_STATE_NOT_PRESENT ==
             pEvent->EventDataUnion.HotSwapEvent.HotSwapState) 
    {
        /* Inform to CPM-G */
        rc = clCmHotSwapCpmUpdate(pRptEntry, CL_CM_BLADE_SURPRISE_EXTRACTION);
    }
    else
    {
        clLog(CL_LOG_ERROR, AREA_HPI, CTX_EVT,
            "Unexpected state [%u] for simple FRU",
             pEvent->EventDataUnion.HotSwapEvent.HotSwapState);
        return CL_CM_ERROR(CL_ERR_INVALID_PARAMETER);
    }
    return rc ;
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                     HPI Event HANDLING                                         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*NOTE:
  -----
  HPI Provides a DAT (DomainAlarmTable ) which maintains the list of alarms in
  the system . HPI is responsible for clearing and appending the alarms to the
  DAT.DAT interface suits best for SNMP kind of apps which walk through the
  Table. 
  Event with severity = major | minor | critical can be regarded as alarms and
  we publish the events with those severities to ASP as alarms. 
 */
extern ClCmEventCorrelatorT clCmEventCorrelationConfigTable[]; 
extern ClCmUserPolicyT cmUserPolicy[];

ClRcT clCmUserPolicyExecute(SaHpiEntityPathT  *pEntity,
                            SaHpiSessionIdT sessionid,
                            SaHpiEventT *pEvent,
                            SaHpiRptEntryT  *pRptEntry, 
                            SaHpiRdrT *pRdr)
{
    ClUint32T index=0, found=0;
    ClRcT rc=CL_OK;
    ClCharT *epath;
    
    epath = clCmEpath2String(pEntity);
    if (NULL == epath)
        epath = "<could not decode>";

    clLog(CL_LOG_DEBUG, AREA_HPI, CTX_EVT, "Searching for user policy handler for [%s]", epath);
    while (SAHPI_ENT_UNSPECIFIED != 
           cmUserPolicy[index].impactedEntity.Entry[0].EntityType)
    {
        if (SAHPI_TRUE==oh_cmp_ep(pEntity,
                                 &(cmUserPolicy[index].impactedEntity)))
        {
           if (NULL!= cmUserPolicy[index].userEventHandler)
           {
                clLog(CL_LOG_INFO, AREA_HPI, CTX_EVT,
                      "Calling user policy handler for [%s]", epath);
                rc=cmUserPolicy[index].userEventHandler(sessionid,
                                                        pEvent,
                                                        pRptEntry,
                                                        pRdr);
                found = 1;
                break;                                        
           }
        }
        index++;
    }
    if (found == 0)
        clLog(CL_LOG_DEBUG, AREA_HPI, CTX_EVT, "No user policy found for [%s]", epath);
    return rc;
}

ClUint32T clCmNumImpactedEntitiesGet( int index, SaHpiEntityPathT  *pReportingEntity)
{
    ClUint32T j=0;
    if(NULL!=clCmEventCorrelationConfigTable[index].pImpactedEntities)
    {
        while (SAHPI_ENT_UNSPECIFIED != 
               clCmEventCorrelationConfigTable[index].pImpactedEntities[j].Entry[0].EntityType)

        j++;

    }
    return j;
}

ClRcT clCmImpactedEntitiesGet(SaHpiEntityPathT  *pReportingEntity,
                              SaHpiEntityPathT  **ppImpactedEntities,
                              ClUint32T *pNumImpactedEntities)
{
    ClUint32T i=0;
    ClUint32T bFound =CL_FALSE;
    while (SAHPI_ENT_UNSPECIFIED != 
           clCmEventCorrelationConfigTable[i].eventReporter.Entry[0].EntityType)
    {
        if(SAHPI_TRUE==oh_cmp_ep(pReportingEntity, 
                            &(clCmEventCorrelationConfigTable[i].eventReporter)))

        {
            bFound =CL_TRUE;
            *pNumImpactedEntities = clCmNumImpactedEntitiesGet(i, pReportingEntity);
            *ppImpactedEntities=(SaHpiEntityPathT *)clHeapAllocate(sizeof(SaHpiEntityPathT)*
                                                               (*pNumImpactedEntities));
            if (NULL==(*ppImpactedEntities))                                                    
            {
                return CL_ERR_NO_MEMORY;
            }
            memcpy(*ppImpactedEntities, 
                   clCmEventCorrelationConfigTable[i].pImpactedEntities,
                   *pNumImpactedEntities);
            break ; 
        }
        i++;
    }
    if (CL_FALSE==bFound)
    {
    }
    return CL_OK;
}


static ClRcT clCmNSIEventHandle( SaHpiSessionIdT sessionId, 
                                 SaHpiEventT *pEvent,
                                 SaHpiRptEntryT  *pRptEntry,
                                 SaHpiRdrT *pRdr)
{
    /* Get the list of Impacted Entities, other than itself */
    SaHpiEntityPathT  *pImpactedEntities=NULL;   
    ClUint32T numEntities=0; 
    ClUint32T counter=0; 
    ClRcT rc;
    ClCharT *epath;
    
    epath = clCmEpath2String(&pRdr->Entity);
    if (NULL == epath)
        epath = "<could not decode>";

    clLog(CL_LOG_DEBUG, AREA_HPI, CTX_EVT,
        "Non-service impacting event received on entity [%s]", epath);
        
    /* SaHpiTextBufferT *epath = NULL; */
    rc = clCmImpactedEntitiesGet(&(pRdr->Entity), &pImpactedEntities, &numEntities);
    if (CL_OK != rc)
    {
        clLog(CL_LOG_ERROR, AREA_HPI, CTX_EVT,
            "Cannot handle NSI event due to memory allocation error, error [0x%x]",
            rc);
        return rc;
    }
    /* call user defined handler(s) */
    rc = clCmUserPolicyExecute(&(pRdr->Entity), sessionId, pEvent, pRptEntry,
                               pRdr);
    if (CL_OK != rc)
    {
        clLog(CL_LOG_ERROR, AREA_HPI, CTX_EVT,
            "Custom NSI event handler failed, error [0x%x] for [%s]",
            rc, eventEpathToString(pEvent, &pRdr->Entity));
    }
    
    if (pImpactedEntities!=NULL)
    {
        clLog(CL_LOG_DEBUG, AREA_HPI, CTX_EVT,
            "Found [%d] indirectly impacted entities, calling their handlers...",
            numEntities);
        for (counter=0; counter < numEntities; counter++)
        {
            rc = clCmUserPolicyExecute(&(pImpactedEntities[counter]), sessionId,
                                       pEvent, pRptEntry, pRdr);
            if (CL_OK != rc)
            {
                clLog(CL_LOG_ERROR, AREA_HPI, CTX_EVT,
                    "Custom NSI event handler failed, error [0x%x] for [%s]",
                    rc, eventEpathToString(pEvent, &(pImpactedEntities[counter])));
            }
        }
        clHeapFree(pImpactedEntities);
    }
    return CL_OK;
}

void clCmInformAMFBySlot(ClUint32T slotId, ClBoolT asserted, SaHpiTimeT timeStamp)
{
    ClRcT rc = CL_OK;
    ClCmCpmMsgT cmCpmMsg;
    const ClCharT *msg;
    
    memset(&cmCpmMsg, 0, sizeof(ClCmCpmMsgT));

    cmCpmMsg.physicalSlot = slotId;
    
    if (asserted)
    {
        cmCpmMsg.cmCpmMsgType = CL_CM_BLADE_NODE_ERROR_REPORT;
        msg = "(imminent) failure";
    }
    else
    {
        cmCpmMsg.cmCpmMsgType = CL_CM_BLADE_NODE_ERROR_CLEAR;
        msg = "recovery";
    }
    rc = clCpmHotSwapEventHandle(&cmCpmMsg);
    if (CL_OK != rc)
    {
        clLog(CL_LOG_ERROR, AREA_HPI, CTX_EVT,
               "Failed to notify AMF about [%s] of slot [%d]", msg, slotId);
        return;
    }
    clLog(CL_LOG_INFO, AREA_HPI, CTX_EVT,
          "CM successfully notified AMF on [%s] of slot [%d]", msg, slotId);
    return;
}
 
void clCmInformAMFByEpath(SaHpiEntityPathT *pEpath, ClBoolT asserted, SaHpiTimeT timeStamp)
{
    ClUint32T slotId = CL_CM_INVALID_PHYSICAL_SLOT ;
    ClCharT *epath_str = NULL;
    
    epath_str = clCmEpath2String(pEpath);
    if (NULL == epath_str)
        epath_str = "<could not decode>";

    slotId = clCmPhysicalSlotFromEntityPath(pEpath);
    if (CL_CM_INVALID_PHYSICAL_SLOT != slotId)
    {
        clLog(CL_LOG_INFO, AREA_HPI, CTX_EVT,
            "Entity [%s] resolves to slot [%d], informing AMF on service impacting event",
            epath_str, slotId);
        clCmInformAMFBySlot(slotId, asserted, timeStamp);
    }
    else 
    {
        clLog(CL_LOG_INFO, AREA_HPI, CTX_EVT,
            "Could not resolve entity path [%s] to a physical slot, ignoring",
            epath_str);
         /* do nothing  as the entity path might not have physical slot in it's
          * entity path*/
    }
}

static ClRcT clCmSIEventHandle( SaHpiSessionIdT sessionid, 
                                SaHpiEventT *pEvent,
                                SaHpiRptEntryT  *pRptentry,
                                SaHpiRdrT *pRdr)
{
    SaHpiEntityPathT  *pImpactedEntities=NULL;   
    ClUint32T numEntities=0; 
    ClUint32T counter=0; 
    ClRcT rc=CL_OK;
    ClCharT *epath;
    ClBoolT event_asserted=0;
    SaErrorT error = SA_OK;
    SaHpiSensorReadingT reading;
    SaHpiEventStateT eventState;
     
    /*
    SaHpiTextBufferT *epath = NULL;
    */ 

    epath = clCmEpath2String(&pRdr->Entity);
    if (NULL == epath)
        epath = "<could not decode>";

    clLog(CL_LOG_INFO, AREA_HPI, CTX_EVT,
        "Service impacting event received on entity [%s]", epath);
    
    /* We are here only if this was a sensor event type */
    assert(pEvent->EventType == SAHPI_ET_SENSOR);
    event_asserted = pEvent->EventDataUnion.SensorEvent.Assertion == SAHPI_TRUE;
    clLog(CL_LOG_INFO, AREA_HPI, CTX_EVT,
        "This indicates the %s of the event", event_asserted ? "assertion" :
                                                               "deassertion");
    /*
     * In case of deassertion of threshold sensor type, we read the sensor
     * state and only deassert if the both MAJOR and CRITICIAL state is cleared.
     */
    if (!event_asserted &&
        pEvent->EventDataUnion.SensorEvent.EventCategory == SAHPI_EC_THRESHOLD)
    {
        if ((error = _saHpiSensorReadingGet( sessionid,
                        pRptentry->ResourceId,
                        pEvent->EventDataUnion.SensorEvent.SensorNum,
                        &reading,
                        &eventState))!= SA_OK ) {
            clLog(CL_LOG_WARNING, AREA_HPI, CTX_EVT, "Cannot read sensor state; using what is indicated in event");
            /* we proceed based on current event_asserted value */
        } else {
            if ((eventState & (SAHPI_ES_LOWER_MAJOR |
                               SAHPI_ES_LOWER_CRIT |
                               SAHPI_ES_UPPER_MAJOR |
                               SAHPI_ES_UPPER_CRIT)) != 0)
            {
                clLog(CL_LOG_WARNING, AREA_HPI, CTX_EVT,
                      "Deassertion event ignored because sensor is still in MAJOR or CRITICAL state");
                /* We bail out as sensor is still in asserted state */
                return rc;
            }
        }
    }
    
    /* Report directly impacted FRUs to AMF */
    clCmInformAMFByEpath(&(pRdr->Entity), event_asserted, pEvent->Timestamp);
    
    /*
     * Report on indirectly impacted FRUs if any. This is a customizable list
     * under <model>/config
     */
    rc = clCmImpactedEntitiesGet(&(pRdr->Entity),
                                 &pImpactedEntities, &numEntities);
    if (CL_OK!=rc)
    {
        clLog(CL_LOG_ERROR, AREA_HPI, CTX_EVT,
            "Cannot derive indirectly impacted entities due to failed memory alloc, error [0x%x]",
            rc);
        return rc;
    }
    if (pImpactedEntities!=NULL)
    {
        clLog(CL_LOG_DEBUG, AREA_HPI, CTX_EVT,
            "Found [%d] indirectly impacted entities, calling their handlers...",
            numEntities);
        for (counter=0;counter < numEntities; counter++)
        {
            clCmInformAMFByEpath(&(pImpactedEntities[counter]),
                                 event_asserted,
                                 pEvent->Timestamp);
        }
        clHeapFree(pImpactedEntities);
    }
    return CL_OK;
}


static ClRcT clCmSensorEventProcess( SaHpiSessionIdT sessionId,
                                     SaHpiEventT *pEvent,
                                     SaHpiRptEntryT *pRptentry,
                                     SaHpiRdrT *pRdr)
{
    ClRcT           rc = CL_OK;
    ClAlarmInfoPtrT pAlarmPayload;
    ClCmSensorEventInfoT *pSensorEventPayload;
    ClUint32T       size = 0;

    /* Publish Sensor Event to CM Event Channel */
    size = sizeof(ClAlarmInfoT)+sizeof(ClCmSensorEventInfoT);

    pSensorEventPayload = clHeapAllocate(sizeof(ClCmSensorEventInfoT));
    if (NULL == pSensorEventPayload)
    {
        return CL_ERR_NULL_POINTER;
    }

    pSensorEventPayload->rptEntry = *pRptentry;
    pSensorEventPayload->rdr = *pRdr;
    pSensorEventPayload->sensorEvent = pEvent->EventDataUnion.SensorEvent;

    pAlarmPayload = clHeapAllocate(size);
    if (NULL == pAlarmPayload)
    {
        return CL_ERR_NULL_POINTER;
    }

    clCpmComponentNameGet(gClCmChassisMgrContext.cmCompHandle, &pAlarmPayload->compName);
    pAlarmPayload->alarmState = ((pEvent->EventDataUnion.SensorEvent.Assertion == SAHPI_TRUE) ?
                                 CL_ALARM_STATE_ASSERT : CL_ALARM_STATE_CLEAR);
    pAlarmPayload->category = CL_ALARM_CATEGORY_EQUIPMENT;
    pAlarmPayload->probCause = CL_ALARM_PROB_CAUSE_EQUIPMENT_MALFUNCTION;
    pAlarmPayload->specificProblem = 0;
    pAlarmPayload->len = sizeof(ClCmSensorEventInfoT);
    memcpy(pAlarmPayload->buff, pSensorEventPayload, sizeof(ClCmSensorEventInfoT));

    rc = clCmPublishEvent(CM_ALARM_EVENT,
                          size,
                          pAlarmPayload);
                          
    if (CL_OK != rc)
    {
        clLog(CL_LOG_CRITICAL, AREA_HPI, CTX_EVT, "Failed to publish sensor event, error[0x%x]", rc);
    }

    clHeapFree(pAlarmPayload);
    clHeapFree(pSensorEventPayload);
    
    /* Handle Non Service Impacting Events by calling user callbacks */
    if (pEvent->Severity > SAHPI_MAJOR)
    {
        rc = clCmNSIEventHandle(sessionId, pEvent, pRptentry, pRdr);

    }
    /* Service Impacting Events Handling */
    else 
    {
        rc = clCmSIEventHandle(sessionId, pEvent, pRptentry, pRdr);

    }
    return rc;
}
