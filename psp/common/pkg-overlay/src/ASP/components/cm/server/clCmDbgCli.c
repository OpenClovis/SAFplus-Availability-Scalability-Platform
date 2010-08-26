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
 * File        : clCmDbgCli.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 * This file contains the implementation of Chassis manager debug cli     
 * commands.                                                                  
 *******************************************************************************/



#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <include/clCmDefs.h>
#include <clDebugApi.h>

#include "clCmUtils.h"

#define AREA_CLI "CLI"

static ClRcT fakeResourceHotswap ( int argc , char **argv , char **retStr );
static ClRcT cmCliHotSwapEvent(int argc, char **argv, char **retStr);

extern void cmCliPrint( char **retstr, char *format , ... );
extern ClCmContextT gClCmChassisMgrContext;
ClHandleT  gCmDebugReg  = CL_HANDLE_INVALID_VALUE;

static ClDebugFuncEntryT cmDbgCliCallbacks [] = 
{
    { (ClDebugCallbackT)fakeResourceHotswap ,
        "_hotswap",
        "Used to simulate the extraction and insertion of the card by s/w"
    },

    {
        (ClDebugCallbackT) cmCliHotSwapEvent,
        "cmHotSwapEvent",
        "Simulate various hot swap events"
    }
};



/* Register the chassis manager with the Debug server */ 
ClRcT cmDebugRegister( ClEoExecutionObjT *peoObj )
{
  
    ClRcT  rc = CL_OK;

    rc = clDebugPromptSet("cm");
    if( CL_OK != rc )
    {
	return rc;
    }
    return clDebugRegister( cmDbgCliCallbacks,
                            (ClUint32T)(sizeof(cmDbgCliCallbacks)/
                            sizeof(cmDbgCliCallbacks[0])), 
                            &gCmDebugReg);
}


/* Deregister the Chassis manager with the Debug server */
ClRcT cmDebugDeregister( ClEoExecutionObjT *peoObj )
{
    return clDebugDeregister( gCmDebugReg ); 
}

ClDebugModEntryT clModTab[] =
{
	{ "cm", "cm", cmDbgCliCallbacks, "CM Commands"},
	{ "", "", 0, ""}
};
/*
   Debug cli commands to simulate the fake hotswap of the resource , should be
   only used for testing purpose only
*/
static ClRcT fakeResourceHotswap ( int argc , char **argv , char **retStr )
{

    ClRcT rc = CL_OK;
    SaHpiResourceIdT resourceId = 0x0;
    SaHpiHsActionT action = 0x0;
    extern char *oh_lookup_error(SaErrorT );
    SaErrorT error = SA_OK;

    /* Allocate maximum possible */ 
    *retStr = clHeapAllocate(DISPLAY_BUFFER_LEN);
    if( *retStr == NULL )
    {
        clLog(CL_LOG_ERROR, AREA_CLI, CL_LOG_CONTEXT_UNSPECIFIED,
            "Could not allocate memory for CLI request");
        return CL_ERR_NO_MEMORY;
    }

    if( argc != 3 )
    {
        cmCliPrint( retStr , "\nUsage: setstate resId  op=<ins or ext> \n"
                             "resId [DEC]- resourceid of the resource  "
                             "op [STRING]- operation can be ins or exs");
        return rc;
    }
    
    resourceId = (SaHpiResourceIdT)atoi(argv[1]);

    if( strcmp(argv[2] , "ins" )== 0)
    {
        action = SAHPI_HS_ACTION_INSERTION;
    }
    else if (strcmp (argv[2] ,"ext")==0 )
    {
        action = SAHPI_HS_ACTION_EXTRACTION;
    }
    else
    {
        cmCliPrint( retStr, "\n Invalid state given ");
        return rc;
    }
    
    if((error = saHpiHotSwapActionRequest( 
                    gClCmChassisMgrContext.platformInfo.hpiSessionId, 
                    resourceId,
                    action 
                     ))!= SA_OK )
    {
        cmCliPrint(retStr, "\nsaHpiHotSwapAction request failed :%s",
                oh_lookup_error(error));
        return rc;
    }
    clLog(CL_LOG_DEBUG, AREA_CLI,  CL_LOG_CONTEXT_UNSPECIFIED,
            "HS action requested on resource id [0x%x]: %d (%s)",
            resourceId, action, argv[2]);
    
    return rc;
}

static ClRcT cmCliHotSwapEvent(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;
    
    /* Allocate maximum possible */ 
    *retStr = clHeapAllocate(DISPLAY_BUFFER_LEN);
    if( *retStr == NULL )
    {
        clLog(CL_LOG_ERROR, AREA_CLI, CL_LOG_CONTEXT_UNSPECIFIED,
            "Could not allocate memory for CLI request");
        return CL_ERR_NO_MEMORY;
    }

    if (argc != 3)
    {
        cmCliPrint(retStr,
                   "Usage : cmHotSwapEvent <node-address> <event-type-string>\n"
                   "Event type string should be one of the following :\n"
                   "1. surprise-extraction.\n"
                   "2. request-extraction.\n"
                   "3. request-insertion.\n"
                   "4. node-error-report.\n"
                   "5. node-error-clear.\n");
        return CL_ERR_INVALID_PARAMETER;
    }
    else
    {
        ClCmCpmMsgT cmCpmMsg;

        memset(&cmCpmMsg, 0, sizeof(ClCmCpmMsgT));

        cmCpmMsg.physicalSlot = atoi(argv[1]);

        if (!strcasecmp(argv[2], "surprise-extraction"))
            cmCpmMsg.cmCpmMsgType = CL_CM_BLADE_SURPRISE_EXTRACTION;
        else if (!strcasecmp(argv[2], "request-extraction"))
            cmCpmMsg.cmCpmMsgType = CL_CM_BLADE_REQ_EXTRACTION;
        else if (!strcasecmp(argv[2], "request-insertion"))
            cmCpmMsg.cmCpmMsgType = CL_CM_BLADE_REQ_INSERTION;
        else if (!strcasecmp(argv[2], "node-error-report"))
            cmCpmMsg.cmCpmMsgType = CL_CM_BLADE_NODE_ERROR_REPORT;
        else if (!strcasecmp(argv[2], "node-error-clear"))
            cmCpmMsg.cmCpmMsgType = CL_CM_BLADE_NODE_ERROR_CLEAR;

        rc = clCpmHotSwapEventHandle(&cmCpmMsg);
        if (CL_OK != rc)
        {
            cmCliPrint(retStr,
                       "CM hot swap event reporting failed, error [%#x]",
                       rc);
            return rc;
        }
    }
    return CL_OK;
}
