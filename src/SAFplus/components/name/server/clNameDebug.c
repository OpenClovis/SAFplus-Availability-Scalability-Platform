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
/*******************************************************************************
 * ModuleName  : name
 * File        : clNameDebug.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains Name Service related DBG CLI commands
 * and function to be called mapping.
 *****************************************************************************/
                                                                                                                             


#include <clEoApi.h>
#include <clDebugApi.h>
#include <clNameIpi.h>

#define NAME_LOG_AREA_DEBUG	"DBG"
#define NAME_LOG_CTX_NAME_REG	"REG"

ClHandleT  gNameDebugReg = CL_HANDLE_INVALID_VALUE;

extern ClRcT cliNSRegister(ClUint32T argc, ClCharT **argv, ClCharT** retStr);
extern ClRcT cliNSComponentDeregister(ClUint32T argc, ClCharT **argv, ClCharT** retStr);
extern ClRcT cliNSServiceDeregister(ClUint32T argc, ClCharT **argv, ClCharT** retStr);
extern ClRcT cliNSContextCreate(ClUint32T argc, ClCharT **argv, ClCharT** retStr);
extern ClRcT cliNSContextDelete(ClUint32T argc, ClCharT **argv, ClCharT** retStr);
extern ClRcT cliNSListEntries(ClUint32T argc, ClCharT **argv, ClCharT** retStr);
extern ClRcT cliNSObjectReferenceQuery(ClUint32T argc, ClCharT **argv, ClCharT** retStr);
extern ClRcT cliNSObjectMapQuery(ClUint32T argc, ClCharT **argv, ClCharT** retStr);
extern ClRcT cliNSAttributeLevelQuery(ClUint32T argc, ClCharT **argv, ClCharT** retStr);
extern ClRcT cliNSAllObjectMapsQuery(ClUint32T argc, ClCharT **argv, ClCharT** retStr);
extern ClRcT cliNSInitialize(ClUint32T argc, ClCharT **argv, ClCharT** retStr);
extern ClRcT cliNSFinalize(ClUint32T argc, ClCharT **argv, ClCharT** retStr);
extern ClRcT cliNSDataDump(ClUint32T argc, ClCharT **argv, ClCharT** retStr);


static ClDebugFuncEntryT nameDebugFuncList[] = {
        {(ClDebugCallbackT) cliNSRegister, "NSRegister", "Register an entry with Name Service"},
        {(ClDebugCallbackT) cliNSComponentDeregister, "NSComponentDeregister", "Deregister all entries associated with a component"},
        {(ClDebugCallbackT)cliNSServiceDeregister, "NSServiceDeregister", "Deregister all entries associated with a component"},
        {(ClDebugCallbackT)cliNSContextCreate, "NSContextCreate", "Create a context"},
        {(ClDebugCallbackT)cliNSContextDelete, "NSContextDelete", "Delete a context"},
        {(ClDebugCallbackT)cliNSListEntries, "NSList", "List all the entries"},
        {(ClDebugCallbackT)cliNSObjectReferenceQuery, "NSObjRefQuery", "Get object reference for a given name"},
        {(ClDebugCallbackT)cliNSObjectMapQuery, "NSObjMapQuery", "Get object mapping for a given name"},
        {(ClDebugCallbackT)cliNSAllObjectMapsQuery, "NSAllObjMapsQuery", "Get all object mappings for a given name"},
        {(ClDebugCallbackT)cliNSAttributeLevelQuery, "NSAttribQuery", "Attribute level query"},
        {(ClDebugCallbackT)cliNSInitialize, "NSInitialize", "Initializes the Name Service Library"},
        {(ClDebugCallbackT)cliNSFinalize, "NSFinalize", "Finalizes the Name Service Library"},
        {(ClDebugCallbackT)cliNSDataDump, "nameDbShow", "Dumps all varibles of server"},
        {NULL, "", ""}
};
                     

ClDebugModEntryT clModTab[] = 
{
	{"name", "name", nameDebugFuncList, "Name Service Module Test Commands"}, 	
	{"", "", 0, ""}
};


ClRcT nameDebugRegister(ClEoExecutionObjT* pEoObj)
{
    ClRcT  rc = CL_OK;

    rc = clDebugPromptSet("NAME");
    if( CL_OK != rc )
    {
        clLogError(NAME_LOG_AREA_DEBUG,NAME_LOG_CTX_NAME_REG,"clDebugPromptSet(): rc[0x %x]", rc);
        return rc;
    }
    return clDebugRegister(nameDebugFuncList, 
           sizeof(nameDebugFuncList)/sizeof(nameDebugFuncList[0]),
                           &gNameDebugReg);
}

                                                                                                                             
ClRcT nameDebugDeregister(ClEoExecutionObjT* pEoObj)
{
    return clDebugDeregister(gNameDebugReg);
}

