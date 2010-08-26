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
 * ModuleName  : hpiEventFilter                                                          
 * File        : clHpiEventFilter.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 * This file contains HPI Event filter library implementation.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <time.h>

#include <clHpiEventFilter.h>

typedef struct ClHpiEventReciever {
    ClHpiEventFilterTableT *event_filter_table;
    ClOsalTaskIdT client_threads;
    ClHandleT handle;
}ClHpiEventRecieverT;    

ClHpiEventRecieverT event_reciever = {
                                        .handle = -1,
};                                        


ClBoolT PmIsEntityOfType(SaHpiEntityPathT *ep, SaHpiEntityTypeT entityType)
{

#ifdef PM_EVENTFILTER_DEBUG
    PM_DEBUG_PRINT(("Current type:%d, Required type:%d\n",ep->Entry[0].EntityType, entityType));
#endif    
    return (ep->Entry[0].EntityType == entityType);

/* Leaf comes first in the Entity Path. Hence commenting out */
#if 0
    for(i=0; (i<SAHPI_MAX_ENTITY_PATH) && (ep->Entry[i].EntityType != SAHPI_ENT_ROOT); i++);

    if(ep->Entry[i].EntityType == SAHPI_ENT_ROOT && ep->Entry[i-1].EntityType == entityType)
            found = CL_TRUE;
#endif
}

void clPmPrintFilter (ClHpiEventFilterT *filter)
{
	ClUint8T valid_ep = filter->entityPath.Entry[0].EntityType == SAHPI_ENT_UNSPECIFIED ? 0:1;

	
	if(filter->entityType != SAHPI_ENT_UNSPECIFIED)
		PM_DEBUG2_PRINT(("Entity Type:%d\n", filter->entityType));
	
	if(valid_ep)
	{
		PM_DEBUG2_PRINT(("Entity Path:"));
		oh_print_ep(&(filter->entityPath), 0);
	}

	if(!((ClInt32T) filter->eventSeverity < 0))
		PM_DEBUG2_PRINT(("Event Severity:%d\n", filter->eventSeverity));
	
	switch(filter->eventType)
	{
		ClHpiSensorEventFilterT *sensorFilter;
		ClHpiHotswapEventFilterT *hotswapFilter;
		ClHpiWatchdogEventFilterT *watchdogTimerFilter;
		
		case PM_HPI_SENSOR_EVENT_FILTER:
                                    PM_DEBUG2_PRINT(("Sensor Filter:\n"));
                                    sensorFilter = (ClHpiSensorEventFilterT *) filter->eventFilter;
             
		 							if(!((ClInt32T)sensorFilter->sensorId < 0))
	                                    PM_DEBUG2_PRINT(("\tSensor Id:%d\n", sensorFilter->sensorId));
			
									if(!((ClInt32T) sensorFilter->sensorType < 0))
										PM_DEBUG2_PRINT(("\tSensor Type:%d\n", sensorFilter->sensorType));
			
									if(!((ClInt8T) sensorFilter->sensorEventCategory < 0))
										PM_DEBUG2_PRINT(("\tSensor EventCategory:%d\n", sensorFilter->sensorEventCategory));

									if(sensorFilter->idString)
										PM_DEBUG2_PRINT(("\tSensor IdString:\"%s\"\n", sensorFilter->idString));
			
									break;

	
		case PM_HPI_WATCHDOG_EVENT_FILTER:

									PM_DEBUG2_PRINT(("Watchdog Event Filter:\n"));
									watchdogTimerFilter = (ClHpiWatchdogEventFilterT *)filter->eventFilter;
										
									if(!((ClInt32T) watchdogTimerFilter->watchdogNum < 0))
										PM_DEBUG2_PRINT(("\tWatchdog Number:%d\n", watchdogTimerFilter->watchdogNum));

									if(!((ClInt32T) watchdogTimerFilter->watchdogUse < 0))
										PM_DEBUG2_PRINT(("\tWatchdog Use:%d\n", watchdogTimerFilter->watchdogUse));

									break;


		case PM_HPI_HOTSWAP_EVENT_FILTER:

									PM_DEBUG2_PRINT(("Hotswap Event Filter:\n"));
									hotswapFilter = (ClHpiHotswapEventFilterT *)filter->eventFilter;
										
									if(!((ClInt32T) hotswapFilter->HotSwapState < 0))
										PM_DEBUG2_PRINT(("\tHotswap State:%d\n", hotswapFilter->HotSwapState));
									if(!((ClInt32T) hotswapFilter->PreviousHotSwapState < 0))
										PM_DEBUG2_PRINT(("\tPrevious HotSwap State:%d\n", hotswapFilter->PreviousHotSwapState));

									break;


		default:
									PM_DEBUG2_PRINT(("Invalid Event Type\n"));
	}				
}

void get_events(SaHpiSessionIdT session_id)
{
	SaErrorT rc = SA_OK;
	SaHpiRptEntryT rpt_entry;
	SaHpiRdrT rdr;
	SaHpiEventT event;
	ClHpiEventFilterT *event_filter_ptr;
    
	if(!event_reciever.event_filter_table)
    {
        ClOsalTaskIdT self;
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Client Thread spawned, but no filters regisetered. Exiting...\n"));
        clOsalSelfTaskIdGet(&self);
        clOsalTaskDelete(self);
    }

	rc = saHpiSubscribe(session_id);
	if(rc != SA_OK)
	{
        ClOsalTaskIdT self;
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Cannot subscribe for events:%s\n",oh_lookup_error(rc)));
		clOsalSelfTaskIdGet(&self);
        clOsalTaskDelete(self);
	}

    PM_DEBUG2_PRINT(("getting events.......: No of filters registered:%d\n",\
					event_reciever.event_filter_table->noOfFilters));	
    
	while(1)
    {
        ClUint32T i;

#ifdef PM_EVENTFILTER_DEBUG
        static int j=0;
        j++;
#endif 

        PM_DEBUG_PRINT(("%d. Event Loop\n",j));

        rc = saHpiEventGet(session_id, SAHPI_TIMEOUT_BLOCK, &event, &rdr, &rpt_entry, NULL);
        if(rc != SA_OK)
        {
            ClOsalTaskIdT self;
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("saHpiEventGet failed:%s\n",oh_lookup_error(rc)));
            clOsalSelfTaskIdGet(&self);
            clOsalTaskDelete(self);
        }          

#ifdef PM_EVENTFILTER_DEBUG        
        PM_DEBUG_PRINT(("Got an Event......\n"));
        oh_print_event(&event, 0);
#endif        

#ifdef PM_EVENTFILTER_DEBUG_LEVEL2
		if(event.EventType == SAHPI_ET_HOTSWAP)
		{
				PM_DEBUG2_PRINT(("\n\n####### At Event Reciever Door Step ######\n"));
        		oh_print_event(&event, 0);
				PM_DEBUG2_PRINT(("############################################\n\n"));

		}
#endif



		for(i=0;i<event_reciever.event_filter_table->noOfFilters;i++)
		{
            ClUint32T valid_ep=0;
			event_filter_ptr = &event_reciever.event_filter_table->filter[i]; 
            //Filter based on EntityPath
		
            PM_DEBUG_PRINT(("Matching EntityType\n"));
            if(event_filter_ptr->entityType != SAHPI_ENT_UNSPECIFIED)
            {
                ClBoolT ofType;
                
                ofType = PmIsEntityOfType(&(rpt_entry.ResourceEntity), event_filter_ptr->entityType);
                if(!ofType)
                {
                /*    PM_DEBUG_PRINT(("Event Entity type:%d\t required:%d\n", event.Source,\
					   	event_filter_ptr->entityType));*/
                    continue;
                }
            }                    
            
            PM_DEBUG_PRINT(("Matching entity path\n")); 

            valid_ep = (event_filter_ptr->entityPath.Entry[0].EntityType == SAHPI_ENT_UNSPECIFIED) ? 0:1;
            if(valid_ep)
            {
                if(!oh_cmp_ep(&(event_filter_ptr->entityPath), &(rpt_entry.ResourceEntity)))
                    continue;
            }       
		    
            PM_DEBUG_PRINT(("Looking for sensor, hotswap or watchdog events\n"));
			switch(event_filter_ptr->eventType)
			{
				ClHpiSensorEventFilterT *sensorFilter;
				ClHpiHotswapEventFilterT *hotswapFilter;
				ClHpiWatchdogEventFilterT *watchdogTimerFilter;

				case PM_HPI_SENSOR_EVENT_FILTER:

								if(event.EventType != SAHPI_ET_SENSOR)
								continue;
										
    
								PM_DEBUG_PRINT(("Found a Sensor Event\n"));
                                sensorFilter = (ClHpiSensorEventFilterT *) event_filter_ptr->eventFilter;
                                        
                                if(!valid_ep && (sensorFilter->idString || !((ClInt32T) sensorFilter->sensorId < 0)))
                                {
	                                 CL_DEBUG_PRINT(CL_DEBUG_INFO, ("Invalid filter:\n"));
                                     clPmPrintFilter(event_filter_ptr);
                                }
											
                                if(!((ClInt32T) sensorFilter->sensorId < 0) &&	\
									sensorFilter->sensorId != event.EventDataUnion.SensorEvent.SensorNum)
									continue;
												
								if(!((ClInt32T) sensorFilter->sensorType < 0) && \
									sensorFilter->sensorType != event.EventDataUnion.SensorEvent.SensorType)
									continue;

								if(!((ClInt8T) sensorFilter->sensorEventCategory < 0) && \
								   	sensorFilter->sensorEventCategory != event.EventDataUnion.SensorEvent.EventCategory)
									continue;
										
                                if(sensorFilter->idString)
                                {
                                	rdr.IdString.Data[rdr.IdString.DataLength]='\0';
                                    if(strcmp((ClCharT *)rdr.IdString.Data, sensorFilter->idString))
                                    continue;
                                }
                                                
								break;

								
           		case PM_HPI_HOTSWAP_EVENT_FILTER:
								if(event.EventType != SAHPI_ET_HOTSWAP)
								continue;
                                            
								PM_DEBUG_PRINT(("Found a hotswap Event \n"));
                                            
								hotswapFilter = (ClHpiHotswapEventFilterT *)event_filter_ptr->eventFilter;
											
                                if(!((ClInt32T)hotswapFilter->HotSwapState < 0) &&\
								  	hotswapFilter->HotSwapState != event.EventDataUnion.HotSwapEvent.HotSwapState)
									continue;
										
								if(!((ClInt32T) hotswapFilter->PreviousHotSwapState < 0) && \
									hotswapFilter->PreviousHotSwapState	!= event.EventDataUnion.HotSwapEvent.PreviousHotSwapState)
									continue;

								break;


				case PM_HPI_WATCHDOG_EVENT_FILTER:
								if(event.EventType != SAHPI_ET_WATCHDOG)
								continue;
                                            
                                PM_DEBUG_PRINT(("Found a watchdog Event\n"));
								watchdogTimerFilter = (ClHpiWatchdogEventFilterT *)event_filter_ptr->eventFilter;

								if(!((ClInt32T) watchdogTimerFilter->watchdogNum < 0) && \
									watchdogTimerFilter->watchdogNum != event.EventDataUnion.WatchdogEvent.WatchdogNum)
									continue;

								if(!((ClInt32T) watchdogTimerFilter->watchdogUse < 0) &&\
								   	watchdogTimerFilter->watchdogUse != event.EventDataUnion.WatchdogEvent.WatchdogUse)
									continue;

								break;
        
				default:
        		                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid Event Type Specified in the filter\n"));

				}

#if 0			
			if(valid_ep && !((ClInt32T)event_filter_ptr->sensorId < 0)) 
            {
                if(rdr.RdrType != SAHPI_SENSOR_RDR || event_filter_ptr->sensorId != rdr.RdrTypeUnion.SensorRec.Num)
                    continue;
            }       
            
            if(!((ClInt32T) event_filter_ptr->sensorType < 0))
            {
                if(rdr.RdrType != SAHPI_SENSOR_RDR || event_filter_ptr->sensorId != rdr.RdrTypeUnion.SensorRec.Type)
                    continue;
            }
            
            if(!((ClInt32T)event_filter_ptr->eventType < 0) && (event_filter_ptr->eventType != event.EventType))
                    continue;
#endif 
            
			PM_DEBUG_PRINT(("Checking event Severity\n"));
            if(!(((ClInt32T)event_filter_ptr->eventSeverity) < 0) && \
							(event_filter_ptr->eventSeverity != event.Severity))
                    continue;

            PM_DEBUG_PRINT(("Calling callback\n")); 
            if(event_filter_ptr->callback)
                (*(event_filter_ptr->callback))(&event, &rpt_entry, &rdr);
            else
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("no callback provided for the filter"));
        }
	}
}	

void *hpi_event_reciever(void *execution_object)
{
    SaHpiSessionIdT session_id;
    SaErrorT rc = SA_OK;
    ClRcT rv=CL_OK;

    rv = clEoMyEoObjectSet((ClEoExecutionObjT *) &execution_object);
    if(rv != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clEoObjectSet failed, cannot recieve events:%x\n",rv));
        return NULL;
    }        
    
    rc = saHpiSessionOpen(SAHPI_UNSPECIFIED_DOMAIN_ID, &session_id, NULL);
    if(rc != SA_OK)
    {
        ClOsalTaskIdT self;
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("saHpiSessionOpen:%s\n",oh_lookup_error(rc)));
        clOsalSelfTaskIdGet(&self);
        clOsalTaskDelete(self);
    }

/*    pthread_cleanup_push(client_thread_cleanup, (void *)&session_id);*/
    
    PM_DEBUG2_PRINT(("Discovery...\n"));
    saHpiDiscover(session_id);
    PM_DEBUG2_PRINT(("Discovery done\n"));
    get_events(session_id);
/*    pthread_cleanup_pop(1);*/
    return NULL;
}

ClRcT clHpiEventFilterRegister(ClHpiEventFilterTableT *filter, ClHandleT *handle)
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT *execution_object;
	ClUint32T filter_size;

    *handle = -1;
    srand(time(0));
    event_reciever.handle=rand();

	if(event_reciever.event_filter_table)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Cannot register: Already registered\n"));
		return CL_ERR_ALREADY_EXIST;
	}
	
	if(!filter || !(filter->filter))
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Cannot Register:Invalid arguments\n"));
		return CL_ERR_INVALID_PARAMETER;
	}

	event_reciever.event_filter_table = clHeapAllocate(sizeof(*(event_reciever.event_filter_table)));
	if(!event_reciever.event_filter_table)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clHeapAllocate failed:%x\n",rc));
		return CL_ERR_NO_MEMORY;
	}	
    
    PM_DEBUG_PRINT(("No of Filters:%d\n", filter->noOfFilters));
    event_reciever.event_filter_table->noOfFilters = filter->noOfFilters;
	filter_size = filter->noOfFilters * sizeof(ClHpiEventFilterT);
	event_reciever.event_filter_table->filter = clHeapAllocate(filter_size);
	if(!event_reciever.event_filter_table->filter)
	{
		clHeapFree(event_reciever.event_filter_table);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clHeapAllocate failed:%x\n",rc));
		return CL_ERR_NO_MEMORY;
	}
	
	memcpy(event_reciever.event_filter_table->filter, filter->filter, filter_size);
	
    rc = clEoMyEoObjectGet(&execution_object);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clEoMyEoObjectGet failed:%x\n",rc));
		clHeapFree(event_reciever.event_filter_table->filter);
		clHeapFree(event_reciever.event_filter_table);
        return rc;
    }        
    rc = clOsalTaskCreateAttached(
                    "Hpi Event Filter Thread",
                    CL_OSAL_SCHED_OTHER,
                    CL_OSAL_THREAD_PRI_NOT_APPLICABLE,/*Priority*/
                    0,/*Stack Size */
                    (void*(*)(void *))hpi_event_reciever,
                    (void *)execution_object,
                    &(event_reciever.client_threads));
    if(rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Cannot create worker thread:%s\n",strerror(errno)));
		clHeapFree(event_reciever.event_filter_table->filter);
		clHeapFree(event_reciever.event_filter_table);
        return rc;
    }        

    *handle = event_reciever.handle;
    return CL_OK;
}    

ClRcT clHpiEventFilterUnregister(ClHandleT handle)
{   
    ClRcT  rc=CL_OK;
	
	if(handle < 0 || event_reciever.handle != handle)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Cannot Unregister: Not registered\n"));
		return CL_ERR_INVALID_PARAMETER;
	}

    event_reciever.handle = -1;

	if(!event_reciever.event_filter_table->filter)
		CL_DEBUG_PRINT(CL_DEBUG_INFO,("Client registered but no filter\n"));
	else		
		clHeapFree(event_reciever.event_filter_table->filter);
	
	clHeapFree(event_reciever.event_filter_table);
    event_reciever.event_filter_table = NULL;
    rc = clOsalTaskDelete(event_reciever.client_threads);
    if(rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clOsalTaskDelete:%s\n",strerror(errno)));
        return rc;
    }   
    
    return CL_OK;
}   


