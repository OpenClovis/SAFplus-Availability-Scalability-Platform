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
 * ModuleName  : fault
 * File        : clFaultServerCmds.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file is used for registering and deregistering
 *						with the debug cli.
 *****************************************************************************/

/* system includes */

/* ASP includes */
#include <clDebugApi.h>
#include <clEoApi.h>

/* fault includes */
#include <clFaultDebug.h>

ClHandleT  gFaultDebugReg = CL_HANDLE_INVALID_VALUE;
static ClDebugFuncEntryT faultDebugFuncList[] =
{
	{(ClDebugCallbackT)clFaultCliDebugGenerateFault, "generateFault", "to generate fault"},
	{(ClDebugCallbackT)clFaultCliDebugCompleteHistoryShow,"queryCompleteFaulthistory", "to query complete fault history"},
	{NULL,"", ""}
};
                                                                                                                             
ClDebugModEntryT clModTab[] =
{
    {"FAULT", "FAULT", faultDebugFuncList, "Fault server commands"},
    {"", "", 0, ""}
};
                                                                                                                             
ClRcT clFaultDebugRegister(ClEoExecutionObjT* pEoObj)
{
    ClRcT  rc = CL_OK;

    rc = clDebugPromptSet("FAULT");
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clDebugPromptSet(): rc[0x %x]", rc));
        return rc;
    }
    return clDebugRegister(faultDebugFuncList,
					   sizeof(faultDebugFuncList)/sizeof(faultDebugFuncList[0]), 
                       &gFaultDebugReg);
}
                                                                                                                             
ClRcT clFaultDebugDeregister(ClEoExecutionObjT* pEoObj)
{
	    return clDebugDeregister(gFaultDebugReg);
}
