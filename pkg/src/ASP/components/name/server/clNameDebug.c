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

ClHandleT  gNameDebugReg = CL_HANDLE_INVALID_VALUE;

extern ClRcT cliNSRegister(int argc, char **argv, char** retStr, ClUint32T flag);
extern ClRcT cliNSComponentDeregister(int argc, char **argv, char** retStr, ClUint32T flag);
extern ClRcT cliNSServiceDeregister(int argc, char **argv, char** retStr, ClUint32T flag);
extern ClRcT cliNSContextCreate(int argc, char **argv, char** retStr, ClUint32T flag);
extern ClRcT cliNSContextDelete(int argc, char **argv, char** retStr, ClUint32T flag);
extern ClRcT cliNSListEntries(int argc, char **argv, char** retStr, ClUint32T flag);
extern ClRcT cliNSListEntries(int argc, char **argv, char** retStr, ClUint32T flag);
extern ClRcT cliNSObjectReferenceQuery(int argc, char **argv, char** retStr, ClUint32T flag);
extern ClRcT cliNSObjectMapQuery(int argc, char **argv, char** retStr, ClUint32T flag);
extern ClRcT cliNSAttributeLevelQuery(int argc, char **argv, char** retStr, ClUint32T flag);
extern ClRcT cliNSAllObjectMapsQuery(int argc, char **argv, char** retStr, ClUint32T flag);
extern ClRcT cliNSInitialize(int argc, char **argv, char** retStr, ClUint32T flag);
extern ClRcT cliNSFinalize(int argc, char **argv, char** retStr, ClUint32T flag);
extern ClRcT cliNSDataDump(int argc, char **argv, char** retStr, ClUint32T flag);


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
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clDebugPromptSet(): rc[0x %x]", rc));
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

