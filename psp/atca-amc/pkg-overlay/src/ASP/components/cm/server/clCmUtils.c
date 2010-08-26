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
 * File        : clCmUtils.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the Utility functions used by Chassis manager
 **************************************************************************/

#include <stdarg.h>
#include <string.h>

#include <oh_utils.h>       /* From OpenHPI package */

#include <clCommon.h>
#include <clOsalApi.h>
#include <clDebugApi.h>
#include <include/clCmDefs.h>

#include "clCmHpiDebug.h"
#include "clCmUtils.h"

extern
ClCmContextT gClCmChassisMgrContext;

static void *
display( void *button );

/* Utility function to display the progressbar */
void
clCmDisplayProgressBar(ClInt16T *button) {

    ClRcT rc = CL_OK;

    if ((rc = clOsalTaskCreateDetached(
                    "DisplayThread",
                    CL_OSAL_SCHED_OTHER,
                    CL_OSAL_THREAD_PRI_NOT_APPLICABLE,
                    0,
                    (void*(*)(void *))display,
                    (void*)button
                    ))!= CL_OK ){
        clLog(CL_LOG_ERROR, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                "Could not create task for progress logs, error [0x%x]", rc);
        return;
    }
    return;
}


static void *
display(void *button)
{

    ClTimerTimeOutT time = {3 , 0};

    while (CL_TRUE)
    {
	    (void)clOsalTaskDelay ( time );
        if (*((ClInt16T*)button)== CL_TRUE)
        {
	        clLog(CL_LOG_INFO, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                "Platform discovery still in progress...");
        }
        else
        {
            break;
        }
    }
    return NULL;
}

/* returns the Range string given the enumeration Value */
char*
clCmSensorRange( SaHpiSensorRangeFlagsT flags ){

	switch(flags){
		case SAHPI_SRF_MIN: return "SAHPI_SRF_MIN";
		case SAHPI_SRF_MAX: return "SAHPI_SRF_MAX";
		case SAHPI_SRF_NORMAL_MIN:return "SAHPI_SRF_NORMAL_MIN";
		case SAHPI_SRF_NORMAL_MAX:return "SAHPI_SRF_NORMAL_MAX";
		case SAHPI_SRF_NOMINAL:return "SAHPI_SRF_NOMINAL";
		default:return "NOT SUPPORTED";
	}

}


/* Returns a string form of the entity path */
char *
clCmEpath2String( SaHpiEntityPathT *epath ){

    static oh_big_textbuffer buffer;
    SaErrorT error = SA_OK;

    if ((error = oh_decode_entitypath(epath, &buffer)) != 0)
    {
        clLog(CL_LOG_ERROR, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "Could not decode entity path: %s", oh_lookup_error(error));
        return NULL;
    }
    return (char*)buffer.Data;
}


/* Returns the physical slot from entity path */
/* This modified mapping function is made to work AMCs in ATCA environments.
** It works by extending the notion of physical slot to cover AMC using a simple
** numbering convention. The numbering convention is made backward compatible
** with the original notion of physical slots in the ATCA base board sense.
** The numbering convention is described below with using 'p' to denote the
** physical slot of ATCA base boards (1-14), and 'a' denotes the AMC slots
** within a given base board (1-8).
**
** The generic mapping formula is:
**
**                      /    p  if addressing a base board,
**                      |
**      PhysicalSlot   <
**                      |
**                      \    100 + 10*p + a if addressing an AMC
**
** The above numbering schema is made to be very easy to translate by humans,
** but has the restriction that it cannot support 16-slot chassis (because
** the PhysicalSlot is mapped one-to-one to node id, and TIPC cannot handle
** node ids larger than 255. For 16-slot chassis the base address (100)
** can be changed to 20, making it somewhat harder to translate by humans but
** make space for 16 slot chassis.
*/

#define _AMC_BASE_ADDRESS 100

ClUint32T
clCmPhysicalSlotFromEntityPath( SaHpiEntityPathT * pEpath )
{
    ClUint32T i;
    ClInt32T amc_slot = -1;

    for(i=0; i<SAHPI_MAX_ENTITY_PATH; i++)
    {
        if( pEpath->Entry[i].EntityType == ATCAHPI_ENT_AMC_SLOT)
        {
            amc_slot = pEpath->Entry[i].EntityLocation;
        }
        else if( pEpath->Entry[i].EntityType == SAHPI_ENT_PHYSICAL_SLOT)
        {
            if (amc_slot == -1)
            {   /* not an AMC, report physical slot */
                return pEpath->Entry[i].EntityLocation;
            }
            else
            {   /* AMC, report according to formula */
                return _AMC_BASE_ADDRESS +
                        10*pEpath->Entry[i].EntityLocation +
                        amc_slot;
            }
        }
    }

    return CL_CM_INVALID_PHYSICAL_SLOT;
}



SaHpiResourceIdT clCmResIdFromEpath(SaHpiEntityPathT * pEpath)
{
    SaHpiResourceIdT resId = SAHPI_UNSPECIFIED_RESOURCE_ID ;
    SaErrorT hpirc = SA_OK;
    SaHpiEntryIdT current = SAHPI_FIRST_ENTRY ,next ;
    SaHpiRptEntryT rptEntry;


    while( current != SAHPI_LAST_ENTRY )
    {
        if((hpirc = _saHpiRptEntryGet (
                        gClCmChassisMgrContext.platformInfo.hpiSessionId ,
                        current ,
                        &next ,
                        &rptEntry
                        ))!= SA_OK )
        {
            clLog(CL_LOG_ERROR, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                "saHpiRptEntryGet failed: %s", oh_lookup_error(hpirc));
            return resId;
        }

        if(SAHPI_TRUE== oh_cmp_ep(&(rptEntry.ResourceEntity),pEpath ) )
        {
            return rptEntry.ResourceId;
        }

        current = next;
        memset(&rptEntry,0,sizeof(SaHpiRptEntryT));
    }

    return resId;
}


ClRcT clCmGetResourceIdFromSlotId(ClUint32T chassisId,
                            ClUint32T physicalSlot,
                            SaHpiResourceIdT *pResourceId)
{
    SaHpiEntryIdT    current = SAHPI_FIRST_ENTRY , next;
    SaHpiRptEntryT   rpt_entry;
    SaErrorT         error = SA_OK;
    ClUint8T         slotFound = CL_FALSE;
    ClUint32T        i;

    memset((void*)&rpt_entry , 0 , sizeof(SaHpiRptEntryT));
    /* Get the resource id from the physical slot id */
    while(current != SAHPI_LAST_ENTRY)
    {

        if((error = _saHpiRptEntryGet (
                        gClCmChassisMgrContext.platformInfo.hpiSessionId ,
                        current ,
                        &next ,
                        &rpt_entry
                        ))!= SA_OK ){
            clLog(CL_LOG_ERROR, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                "saHpiRPTEntryGet failed: %s", oh_lookup_error(error));
            return CL_CM_ERROR ( CL_ERR_CM_HPI_ERROR );
        }

        /* for each rpt entry get the physical slot id and compare it with the
         ** physical slot id in the entity path */
        for( i =0 ; i< SAHPI_MAX_ENTITY_PATH ;i++ )
        {
            if( rpt_entry.ResourceEntity.Entry[i].EntityType
                    == SAHPI_ENT_PHYSICAL_SLOT)
             {
                if(physicalSlot ==
                        rpt_entry.ResourceEntity.Entry[i].EntityLocation)
                {
                    slotFound = CL_TRUE;
                    *pResourceId = (rpt_entry.ResourceId);
                    break;
                }
            }
        }

        if(slotFound ) break;
        current = next;
        memset(&rpt_entry,'\0',sizeof(SaHpiRptEntryT));
    }

    if(!slotFound )
    {
        clLog(CL_LOG_ERROR, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "Slot [%d] not found in the RPT Table", physicalSlot );
        *pResourceId = SAHPI_UNSPECIFIED_RESOURCE_ID;
        return CL_CM_ERROR ( CL_ERR_INVALID_PARAMETER );
    }

    return CL_OK;
}
/* Utility function to print the messages on to the CLI Console */
void
cmCliPrint( char **retstr, char *format , ... ){
    size_t len = strlen(*retstr);
	va_list ap;
	va_start(ap, format);
	vsnprintf(*retstr+len, DISPLAY_BUFFER_LEN-len-1, format, ap);
	va_end(ap);
	return;
}
