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
 * File        : clCmEventInterface.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
* This file contains the interface implementation of the chassis manager 
* with ASP.
****************************************************************************/

#include <SaHpi.h>
#include <clCmApi.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clEventApi.h>
#include <include/clCmDefs.h>
#include <string.h>
#include <unistd.h>

#define AREA_EVT "EVT" /* log area for this module */

/* Why do I need to  provide callback if i dont use it :-S */
void dummy ( void ) { return;  }


extern ClCmContextT gClCmChassisMgrContext;


/* Called by Chassis manager to Initialize the Event Interface */
ClRcT clCmEventInterfaceInitialize(void )
{

	ClRcT rc = CL_OK;
	ClVersionT eventLibVersion = CL_EVENT_VERSION;
	ClEventCallbacksT callbacks= 
    {
		(ClEventChannelOpenCallbackT) dummy, 
		(ClEventDeliverCallbackT)     dummy 
	};
	ClNameT cmEvtChannelName =   {
		/*strlen doen not includes '\0'*/
		(ClUint16T)strlen(CL_CM_EVENT_CHANNEL),
		CL_CM_EVENT_CHANNEL 
	};


	memset((void*)&gClCmChassisMgrContext.eventInfo.EventLibHandle,
			0, sizeof(ClEventHandleT ));
	memset((void*)&gClCmChassisMgrContext.eventInfo.EventChannelHandle,
			0, sizeof(ClEventChannelHandleT));
	
	rc = clEventInitialize( &gClCmChassisMgrContext.eventInfo.EventLibHandle, 
                            &callbacks, 
                            &eventLibVersion);

	if ( rc != CL_EVENT_ERR_ALREADY_INITIALIZED && rc != CL_OK )
    {
		clLog(CL_LOG_ERROR, AREA_EVT, CL_LOG_CONTEXT_UNSPECIFIED,
            "Event client library initialization failed, error [0x%x]", rc);
		return rc;
	}
	
	rc = clEventChannelOpen( gClCmChassisMgrContext.eventInfo.EventLibHandle,
                             &cmEvtChannelName, 
                             (ClEventChannelOpenFlagsT)
                             (CL_EVENT_CHANNEL_CREATE | CL_EVENT_CHANNEL_PUBLISHER 
                             | CL_EVENT_GLOBAL_CHANNEL),
                             -1,
                             &gClCmChassisMgrContext.eventInfo.EventChannelHandle );

	if ( rc != CL_EVENT_ERR_EXIST && rc != CL_OK )
    {
		clLog(CL_LOG_ERROR, AREA_EVT, CL_LOG_CONTEXT_UNSPECIFIED,
            "Could not open event channel for CM, error [0x%x]", rc);
		clEventFinalize(gClCmChassisMgrContext.eventInfo.EventLibHandle);
		return rc;
	}

    /* Allocate the event */
	memset((void*)&gClCmChassisMgrContext.eventInfo.EvtHandle, 
			0, sizeof(ClEventHandleT));

	if ((rc=clEventAllocate(gClCmChassisMgrContext.eventInfo.EventChannelHandle, 
                            &gClCmChassisMgrContext.eventInfo.EvtHandle ))!= CL_OK)
   {
		clLog(CL_LOG_ERROR, AREA_EVT, CL_LOG_CONTEXT_UNSPECIFIED,
            "Event allocation failed, error [0x%x]", rc);
		clEventChannelClose(gClCmChassisMgrContext.eventInfo.EventChannelHandle);
		clEventFinalize(gClCmChassisMgrContext.eventInfo.EventLibHandle);
		return rc;
	}

	gClCmChassisMgrContext.eventInfo.eventIfaceInitDone = CL_TRUE;
    clLog(CL_LOG_DEBUG, AREA_EVT, CL_LOG_CONTEXT_UNSPECIFIED,
        "Event channel successfully opened for CM event publication");
	return rc;
}


/* Called by Chassis manager to Finalize the Event Interface */
ClRcT clCmEventInterfaceFinalize( void )
{

	ClRcT rc = CL_OK;
	
	if( gClCmChassisMgrContext.eventInfo.eventIfaceInitDone )
    {
	    clEventFree( gClCmChassisMgrContext.eventInfo.EvtHandle );
		clEventChannelClose( gClCmChassisMgrContext.eventInfo.EventChannelHandle );
		clEventFinalize( gClCmChassisMgrContext.eventInfo.EventLibHandle );
	}

	gClCmChassisMgrContext.eventInfo.eventIfaceInitDone = CL_FALSE;
    clLog(CL_LOG_DEBUG, AREA_EVT, CL_LOG_CONTEXT_UNSPECIFIED,
        "Event channel closed");
	return rc;
}



ClRcT clCmPublishEvent( CmEventIdT cmEventId,
                        ClUint32T payLoadLen,
                        void *payload )
{
	ClRcT rc = CL_OK;
    ClEventIdT eventId; 
    ClNameT chassisManagerEvent ;
    ClEventPatternT evtPattern ;
    ClEventPatternArrayT evtPatternArray;
    static ClNameT cmEventMap[CM_MAX_EVENTS] = { 
        {.length=sizeof(CL_CM_FRU_STATE_TRANSITION_EVENT_STR)-1,
         .value=CL_CM_FRU_STATE_TRANSITION_EVENT_STR
        },
        {.length=sizeof(CL_CM_ALARM_EVENT_STR)-1,
         .value=CL_CM_ALARM_EVENT_STR
        },
        {.length=sizeof(CL_CM_WATCHDOG_EVENT_STR)-1,
         .value=CL_CM_WATCHDOG_EVENT_STR
        },
    };

    memset(&chassisManagerEvent, 0, sizeof(ClNameT));

    if(cmEventId >= CM_MAX_EVENTS)
    {
        clLog(CL_LOG_ERROR, AREA_EVT, CL_LOG_CONTEXT_UNSPECIFIED,
              "Invalid event type [%u]", cmEventId);
        return CL_CM_ERROR(CL_ERR_INVALID_PARAMETER);
    }

    memcpy(&chassisManagerEvent, &cmEventMap[cmEventId], sizeof(chassisManagerEvent));
	
    evtPattern.allocatedSize = 0;
    evtPattern.patternSize = (ClSizeT)chassisManagerEvent.length + 1;
    evtPattern.pPattern = (ClUint8T *)chassisManagerEvent.value;

	evtPatternArray.allocatedNumber = 0;
    evtPatternArray.patternsNumber = 1;
    evtPatternArray.pPatterns = &evtPattern;

	
	if((rc = clEventAttributesSet(
					gClCmChassisMgrContext.eventInfo.EvtHandle, 
					&evtPatternArray,
					0x01,
					0,
					&chassisManagerEvent
					))!= CL_OK ){
		clLog(CL_LOG_ERROR, AREA_EVT, CL_LOG_CONTEXT_UNSPECIFIED,
            "Failed to set event attributes, error [0x%x]", rc);
		return rc;
	}
	
	if((rc = clEventPublish( 
					gClCmChassisMgrContext.eventInfo.EvtHandle, 
					(void*)payload,
					payLoadLen,
					&eventId
                    ))!= CL_OK){
		clLog(CL_LOG_ERROR, AREA_EVT, CL_LOG_CONTEXT_UNSPECIFIED,
            "Could not publish event, error [0x%x]", rc);
		return rc;
	}
    clLog(CL_LOG_DEBUG, AREA_EVT, CL_LOG_CONTEXT_UNSPECIFIED,
            "Event published");
	return rc;
}
