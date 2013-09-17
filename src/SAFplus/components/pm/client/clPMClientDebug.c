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
 * ModuleName  : PM 
 * File        : clPMClientDebug.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *      This module contains the PM client debug cli implementation. 
 *****************************************************************************/

/* Standard Inculdes */
#include <string.h>

/* ASP Includes */
#include <clCommon.h>
#include <clVersionApi.h>
#include <clIocApi.h>
#include <clLogApi.h>
#include <clEoApi.h>
#include <clCpmApi.h>
#include <clDebugApi.h>
#include <clPMErrors.h>
#include <clPMApi.h>
#include <clCorNotifyApi.h>
#include <clCorUtilityApi.h>

extern ClTimerHandleT gClPMTimerHandle;

static void clPMClientDebugCliPrint(ClCharT* str, ClCharT** pRetStr)
{
   *pRetStr = clHeapAllocate(strlen(str) + 1);
   if (! *pRetStr)
   {
       clLogError("PM", "CLI", "Failed to allocate memory. rc [0x%x]",
               CL_PM_RC(CL_ERR_NO_MEMORY));
       return;
   }

   snprintf(*pRetStr, strlen(str)+1, str);
   return;
}

static ClRcT clPMCliPMStart(ClUint32T argc, ClCharT* argv[], ClCharT** retStr)
{
    ClRcT rc = CL_OK;
    ClCorMOIdListT moIdList = {0};
    ClCharT pmStr[CL_MAX_NAME_LENGTH] = {0};
    ClCorMOIdT moId = {{{0}}};
    SaNameT moIdname = {0};

    if (argc != 2)
    {
        clPMClientDebugCliPrint("Usage : pmStart <moId> \n"
                "moId [STRING] : Managed Object Identifier, eg. \\Chassis:0\\pmResource:0\n", retStr);
        return CL_OK;
    }

    saNameSet(&moIdname, argv[1]);

    rc = clCorMoIdNameToMoIdGet(&moIdname, &moId);
    if (rc != CL_OK)
    {
        clLogError("PM", "CLI", "Failed to xlate moId. rc [0x%x]", rc);
        snprintf(pmStr, CL_MAX_NAME_LENGTH, "Invalid moId. rc [0x%x]", rc);
        clPMClientDebugCliPrint(pmStr, retStr);
        return rc;
    }

    moIdList.moIdCnt = 1;
    memcpy(&moIdList.moId[0], &moId, sizeof(ClCorMOIdT));

    rc = clPMStart(&moIdList);
    if (rc != CL_OK)
    {
        clLogError("PM", "CLI", "Failed to start PM operation on moId : [%s]. rc [0x%x]",
                argv[1], rc);
        snprintf(pmStr, CL_MAX_NAME_LENGTH, 
                "Failed to start PM operation on moId : [%s]. rc [0x%x]", argv[1], rc);
        clPMClientDebugCliPrint(pmStr, retStr);
        return rc;
    }

    return CL_OK;
}

static ClRcT clPMCliPMStop(ClUint32T argc, ClCharT* argv[], ClCharT** retStr)
{
    ClRcT rc = CL_OK;
    ClCorMOIdListT moIdList = {0};
    ClCharT pmStr[CL_MAX_NAME_LENGTH] = {0};
    ClCorMOIdT moId = {{{0}}};
    SaNameT moIdname = {0};

    if (argc != 2)
    {
        clPMClientDebugCliPrint("Usage : pmStop <moId> \n"
                "moId [STRING] : Managed Object Identifier, eg. \\Chassis:0\\pmResource:0\n", retStr);
        return CL_OK;
    }

    saNameSet(&moIdname, argv[1]);

    rc = clCorMoIdNameToMoIdGet(&moIdname, &moId);
    if (rc != CL_OK)
    {
        clLogError("PM", "CLI", "Failed to xlate moId. rc [0x%x]", rc);
        snprintf(pmStr, CL_MAX_NAME_LENGTH, "Invalid moId. rc [0x%x]", rc);
        clPMClientDebugCliPrint(pmStr, retStr);
        return rc;
    }

    moIdList.moIdCnt = 1;
    memcpy(&moIdList.moId[0], &moId, sizeof(ClCorMOIdT));

    rc = clPMStop(&moIdList);
    if (rc != CL_OK)
    {
        clLogError("PM", "CLI", "Failed to stop PM operation on moId : [%s]. rc [0x%x]",
                argv[1], rc);
        snprintf(pmStr, CL_MAX_NAME_LENGTH, 
                "Failed to stop PM operation on the given moId.");
        clPMClientDebugCliPrint(pmStr, retStr);
        return rc;
    }

    return CL_OK;
}

static ClRcT clPMCliPMConfigIntervalSet(ClUint32T argc, ClCharT* argv[], ClCharT** retStr)
{
    ClRcT rc = CL_OK;
    ClCharT pmStr[CL_MAX_NAME_LENGTH] = {0};
    ClTimerTimeOutT timerInterval = {0};

    if (argc != 2)
    {
        clPMClientDebugCliPrint("Usage : pmConfigInterval <interval> \n"
                "interval (milli secs) : pm configuration interval", retStr);
        return CL_OK;
    }

    timerInterval.tsSec = 0;
    timerInterval.tsMilliSec = atoi(argv[1]);
     
    rc = clTimerUpdate(gClPMTimerHandle, timerInterval);
    if (rc != CL_OK)
    {
        clLogError("PM", "CLI", "Failed to update PM config timer interval. rc [0x%x]", rc);
        snprintf(pmStr, CL_MAX_NAME_LENGTH, "Failed to update PM config timer interval. rc [0x%x]", rc);
        clPMClientDebugCliPrint(pmStr, retStr);
        return rc;
    }

    return CL_OK;
}

static ClDebugFuncEntryT ClPMClientDebugCliFuncList[] = {
    {clPMCliPMStart, "pmStart", "Starts PM operation on the given moId."},
    {clPMCliPMStop, "pmStop", "Stops PM operation on the given moId."},
    {clPMCliPMConfigIntervalSet, "pmConfigInterval", "Updates the PM config interval duration"}
};

ClRcT clPMClientDebugRegister(ClHandleT* pDebugHandle)
{
    ClRcT rc = CL_OK;
    ClCpmHandleT cpmHandle = 0;
    SaNameT compName = {0};

    rc = clCpmComponentNameGet(cpmHandle, &compName);
    if (rc != CL_OK)
    {
        clLogError("PM", "CLI", "Failed to get component name from CPM. rc [0x%x]", rc);
        return rc;
    }

    rc = clDebugRegister(ClPMClientDebugCliFuncList, 
            sizeof(ClPMClientDebugCliFuncList)/sizeof(ClDebugFuncEntryT), pDebugHandle);
    if (rc != CL_OK)
    {
        clLogError("PM", "CLI", "Failed to register with debug cli. rc [0x%x]", rc);
        return rc;
    }

    return CL_OK;
}

ClRcT clPMClientDebugUnregister(ClHandleT debugHandle)
{
    ClRcT rc = CL_OK;

    rc = clDebugDeregister(debugHandle);
    if (rc != CL_OK)
    {
        clLogError("PM", "CLI", "Failed to unregister debug cli. rc [0x%x]", rc);
        return rc;
    }

    return CL_OK;
}
