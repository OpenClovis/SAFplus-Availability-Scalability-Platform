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
 * File        : clAlarmClientDebug.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 *
 *          This file provide implementation to provide CLI capability to the 
 *          alarm client.
 *          This C file SHOULD NOT BE MODIFIED BY THE USER
 *
 *****************************************************************************/

/* Standard Inculdes */
#include <string.h>  /* strcpy is needed for eo name */
#include <time.h>
#include <sys/time.h>

/* ASP Includes */
#include <clCommon.h>
#include <clAlarmApi.h>
#include <clAlarmDefinitions.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clCorUtilityApi.h>
#include <clCorMetaData.h>
#include <clAlarmErrors.h>
#include <clCpmApi.h>


/************************************
 Function declaration.
**************************************/  
static ClRcT _clAlarmCliAlarmRaise (ClUint32T argc, 
        ClCharT** argv, ClCharT** ret);


/*************************************
     ALARM CLIENT CLI FUNCTION TABLE
***************************************/     
static ClDebugFuncEntryT gAlarmDebugFuncTable [] = 
{
    {_clAlarmCliAlarmRaise, "alarmRaise", "CLI used for raising the alarm"},
    {NULL, "", ""}
};

/**
 * Internal function to allocate the return string 
 * and copy the data to the it. This return string
 * will be used in the debug cli command display string.
 */ 
static void clAlarmCliStrPrint(ClCharT* str, ClCharT**retStr)
{
    *retStr = clHeapAllocate(strlen(str)+1);
    if(NULL == *retStr)
    {
        clLogError("ALM", "DBG", "Failed to allocate the memory.");
        return;
    }

    sprintf(*retStr, str);
    return;
}

/**
 * Internal function registered as the callback function of the 
 * alarm raise alarm client CLI command.
 */ 
static ClRcT 
_clAlarmCliAlarmRaise (ClUint32T argc, ClCharT** argv, ClCharT** retStr)
{
    ClRcT           rc = CL_OK;
    ClNameT         moIdName = {0};
    ClCharT         almStr[CL_COR_MAX_NAME_SZ] = {0};
    ClUint32T       i = 0;
    ClUint64T       size = 0;
    ClAlarmInfoT    *pAlarmInfo = NULL;
    ClAlarmHandleT  alarmHandle = 0;

    if (argc < 6)
    {
        clAlarmCliStrPrint("Usage: alarmRaise <Resource MOID> <Probable Cause> <Specific Problem> <Severity> <Alarm State> <Additional Info> \n"
                            "Resource MOID    : MOID of the resource on which alarm is configured. \n"
                            "                  \\Chassis:0\\GigeBlade:2\\GigePort:0 or \\0x10001:0\\0x10011:2\\0x10013:0 \n"
                            "Probable Cause   : Probable Cause of the alarm \n"
                            "Specific Problem : Specific Problem of the alarm. It should be zero if no specific problem exist.  \n"
                            "Severity         : Severity of the alarm. \n"
                            "Alarm State      : 1 - RAISED, 0 - CLEARED \n"
                            "Additional Info  : Additional information about the alarm. This parameter is optional. \n", 
                            retStr);
        return CL_ALARM_RC(CL_ALARM_ERR_INVALID_PARAM);
    }
    

    if (argc > 6)
    {
        clLogTrace("ALM", "DBG", "The payload information is given...");

        for ( i = 6 ; i < argc ; i++)
            size += strlen(argv[i]);

        pAlarmInfo = clHeapAllocate (sizeof(ClAlarmInfoT) + size + (argc - 6));
        if (NULL == pAlarmInfo)
        {
            clLogError("ALM", "DBG", 
                    "Failed to allocate the memory for alarm structure");
            sprintf(almStr, "Execution Failed: 0x%x", rc);
            clAlarmCliStrPrint(almStr, retStr);
            return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
        }

        for ( i = 6; i < argc; )
        {
            clLogTrace("ALM", "DBG", "The alarm payload at index [%d] is [%s]",
                    i, argv[i]);

            strncat((ClPtrT)pAlarmInfo->buff, argv[i], strlen(argv[i]));
            i++;

            if ( i < argc)
                strncat((ClPtrT)pAlarmInfo->buff, " ", 1);
        }

        pAlarmInfo->len = sizeof (ClAlarmInfoT) + size + (argc - 6);
    }
    else
    {
        pAlarmInfo = clHeapAllocate(sizeof(ClAlarmInfoT));
        if (NULL == pAlarmInfo)
        {
            clLogError("ALM", "DBG", 
                    "Failed to allocate the memory for alarm info structure.");
            return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
        }

        pAlarmInfo->len = 0;
    }

    if (strlen(argv[1]) >= (sizeof(moIdName) - 1))
    {
        sprintf(almStr, "Execution Failed: [0x%x]", 
                CL_ALARM_RC(CL_ALARM_ERR_INVALID_PARAM));
        clAlarmCliStrPrint(almStr, retStr);
        rc = CL_ALARM_RC(CL_ALARM_ERR_INVALID_PARAM);
        goto handleError;
    }

    strncpy(moIdName.value, argv[1], CL_MAX_NAME_LENGTH -1); 
    moIdName.length = strlen(moIdName.value) + 1;
            
    rc = clCorMoIdNameToMoIdGet(&moIdName, &pAlarmInfo->moId);
    if (CL_OK != rc)
    {
        sprintf(almStr, "Exectution Result: Failed [0x%x] ", rc);
        clAlarmCliStrPrint(almStr, retStr);
        goto handleError;
    }

    pAlarmInfo->probCause = strtol (argv[2], NULL, 0);
    pAlarmInfo->specificProblem = strtol (argv[3], NULL, 0);
    pAlarmInfo->severity = strtol(argv[4], NULL, 0);
    pAlarmInfo->alarmState = strtol (argv[5], NULL, 0);

    rc = clAlarmRaise(pAlarmInfo, &alarmHandle);
    if (CL_OK != rc)
    {
        sprintf(almStr, "Execution Result: Failed [0x%x]", rc);
        clAlarmCliStrPrint(almStr, retStr);
        goto handleError;    
    }

    clAlarmCliStrPrint("Execution Result: Passed", retStr);

handleError:
    clHeapFree(pAlarmInfo);

    return rc;    
}


/**
 * Function to register the CLI commands to be run from the alarm
 * client. The pointer to the alarm handle would be supplied by the 
 * the user which will be populated by the alarm client.
 */ 
ClRcT 
clAlarmClientDebugRegister (CL_OUT ClHandleT * pAlarmDebugHandle)
{
    ClRcT   rc = CL_OK;
    ClNameT cliName = {0};
    ClCpmHandleT cpmHandle = {0};
    ClCharT debugPrompt[CL_DEBUG_COMP_PROMPT_LEN] = {0};
    

    clDbgIfNullReturn (pAlarmDebugHandle, CL_CID_ALARMS);

    rc = clCpmComponentNameGet ( cpmHandle, &cliName );
    if ( CL_OK != rc )
    {
        clLogError ( "ALM", "DRG", "Failed to get the component Name. rc[0x%x]", rc );
        return rc;
    }

    clLogTrace ( "ALM", "DRG", 
            "Registering the Alamr CLI commands for the component [%s]", cliName.value );

    /* Assuming that the component name is not taking whole of 256 bytes of cliName.value */
    strncpy ( debugPrompt, cliName.value, CL_DEBUG_COMP_PROMPT_LEN - strlen("_Alm_Cli") -1);
    strcat ( debugPrompt, "_Alm_Cli" );

    clLog ( CL_LOG_SEV_TRACE, "ALM", "DRG", "The CLI command prompt is [%s]", debugPrompt );

    if (NULL == pAlarmDebugHandle)
    {
        clLogError("ALM", "DBG", "The debug handle passed is NULL.");
        return CL_ALARM_RC(CL_ALARM_ERR_NULL_POINTER);
    }

    rc = clDebugRegister (gAlarmDebugFuncTable,
                          sizeof(gAlarmDebugFuncTable) / sizeof(ClDebugFuncEntryT),
                          pAlarmDebugHandle);
    if ( CL_OK != rc)
    {
        clLogError("ALM", "DBG", "Failed while registering the CLI commands. rc[0x%x]", rc);
        return rc;
    }

    /* Setting the debug prompt.  */
    rc = clDebugPromptSet ( debugPrompt );
    if ( CL_OK != rc )
    {
        clLogError ( "ALM", "DRG", 
                "Failed while setting the prompt for the alarm client CLI. rc[0x%x]", 
                rc );
        return rc;
    }

    clLogInfo ( "ALM", "DRG", 
            "Successfully completed the CLI registration [%s]", debugPrompt );


    return rc;
}



/**
 * Function to deregister the command registered 
 * earlier using the API clAlarmDebugRegister().
 */ 

ClRcT 
clAlarmClientDebugDeregister (CL_IN ClHandleT alarmDebugHandle)
{
    ClRcT   rc = CL_OK;

    rc = clDebugDeregister(alarmDebugHandle);
    if (CL_OK != rc)
    {
        clLogError("ALM", "DBG", 
                "Failed while deregistering the alarm CLI commands. rc[0x%x]", rc);
        return rc;
    }

    return rc;
}
