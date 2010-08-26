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
 * ModuleName  : alarm
 * File        : clAlarmServerCmds.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <clDebugApi.h>
#include <clEoApi.h>
#include <clAlarmDebug.h>

/* Added showAllAlarmHandles and showAlarmInfo */

ClHandleT  gAlarmDebugReg = CL_HANDLE_INVALID_VALUE;
static ClDebugFuncEntryT clAlarmDebugFuncList[] = 
{
{(ClDebugCallbackT)clAlarmCliRaise, "raiseAlarm", "to raise an alarm"},
{(ClDebugCallbackT)clAlarmCliClear, "clearAlarm", "to clear an alarm"},
{(ClDebugCallbackT)clAlarmCliHandlesShow, "showAllAlarmHandles", "to show handles of raised alarms"},
{(ClDebugCallbackT)clAlarmCliInfoShow, "showAlarmInfo", "to show detail alarm info"},
{(ClDebugCallbackT)clAlarmCliQuery, "queryAlarm", "find assert or clear for a alarmid aka probable cause"},
{(ClDebugCallbackT)clAlarmCliShowRaisedAlarms, "showRaisedAlarms", "displays published alarms."},
{(ClDebugCallbackT)clAlarmCliShowAssociatedAlarms, "showAssociatedAlarms", "displays the associated alarms for a resource."},
{(ClDebugCallbackT)clAlarmCliShowAlarmSeverityList, "showAlarmSeverityList", "displays the list of alarm severities."},
{NULL,"", ""}};

ClDebugModEntryT clModTab[] = 
{
    {"ALARM", "ALARM", clAlarmDebugFuncList, "Alarm server commands"},     
    {"", "", 0, ""}
};

ClRcT clAlarmDebugRegister(ClEoExecutionObjT* pEoObj)
{
    ClRcT rc = CL_OK;

    rc = clDebugPromptSet("ALARM");
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clDebugPromptSet(): rc[0x %x]", rc));
        return rc;
    }
   return clDebugRegister(clAlarmDebugFuncList, 
                          sizeof(clAlarmDebugFuncList)/sizeof(clAlarmDebugFuncList[0]), 
                          &gAlarmDebugReg);
}
                                                                                                                             
ClRcT clAlarmDebugDeregister(ClEoExecutionObjT* pEoObj)
{
    return clDebugDeregister(gAlarmDebugReg);
}

