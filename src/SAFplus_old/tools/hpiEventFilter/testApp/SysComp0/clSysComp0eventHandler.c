/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * For more  information, see  the file  COPYING provided with this
 * material.
 */
#include "clSysComp0eventHandler.h" 
#include <clOsalApi.h>

void event_callback(SaHpiEventT *eventPtr, SaHpiRptEntryT *rptEntryPtr, SaHpiRdrT *rdrPtr)
{
    printf("Test App:Recieved an Event\n");
    clOsalPrintf("Entity Associated with the event:");
    oh_print_ep(&rptEntryPtr->ResourceEntity, 0);
    oh_print_event(eventPtr, 0);
#if 0
    if(clCmIsSystemBoard(&(rpt_entry_ptr->ResourceEntity)))
    {
#endif        
    return;
}    

void chassis_wide_temperature_callback(SaHpiEventT *eventPtr, SaHpiRptEntryT *rptEntryPtr, SaHpiRdrT *rdrPtr)
{
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Chassis wide Temperature callback\n"));
    clOsalPrintf("Entity Associated with the event:");
    oh_print_ep(&rptEntryPtr->ResourceEntity, 0);
    oh_print_event(eventPtr, 0);
    return;
}

void fantray_callback(SaHpiEventT *eventPtr, SaHpiRptEntryT *rptEntryPtr, SaHpiRdrT *rdrPtr)
{
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("FanTray callback\n"));
    clOsalPrintf("Entity Associated with the event:");
    oh_print_ep(&rptEntryPtr->ResourceEntity, 0);
    oh_print_event(eventPtr, 0);
    return;
}

void temperature_callback(SaHpiEventT *eventPtr, SaHpiRptEntryT *rptEntryPtr, SaHpiRdrT *rdrPtr)
{
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Temperature callback\n"));
    clOsalPrintf("Entity Associated with the event:");
    oh_print_ep(&rptEntryPtr->ResourceEntity, 0);
    oh_print_event(eventPtr, 0);
    return;
}

void current_callback(SaHpiEventT *eventPtr, SaHpiRptEntryT *rptEntryPtr, SaHpiRdrT *rdrPtr)
{
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("current callback\n"));
    clOsalPrintf("Entity Associated with the event:");
    oh_print_ep(&rptEntryPtr->ResourceEntity, 0);
    oh_print_event(eventPtr, 0);
    return;
}

void voltage_callback(SaHpiEventT *eventPtr, SaHpiRptEntryT *rptEntryPtr, SaHpiRdrT *rdrPtr)
{
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Voltage callback\n"));
    clOsalPrintf("Entity Associated with the event:");
    oh_print_ep(&rptEntryPtr->ResourceEntity, 0);
    oh_print_event(eventPtr, 0);
    return;
}

void processor_callback(SaHpiEventT *eventPtr, SaHpiRptEntryT *rptEntryPtr, SaHpiRdrT *rdrPtr)
{
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Processor callback\n"));
    clOsalPrintf("Entity Associated with the event:");
    oh_print_ep(&rptEntryPtr->ResourceEntity, 0);
    oh_print_event(eventPtr, 0);
    return;
}



void hotswap_callback(SaHpiEventT *eventPtr, SaHpiRptEntryT *rptEntryPtr, SaHpiRdrT *rdrPtr)
{
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("@@@@@@@@@@@@@@@@@@@@@@@@@@HotSwap callback\n"));
    clOsalPrintf("Entity Associated with the event:");
    oh_print_ep(&rptEntryPtr->ResourceEntity, 0);
    oh_print_event(eventPtr, 0);
    return;
}
