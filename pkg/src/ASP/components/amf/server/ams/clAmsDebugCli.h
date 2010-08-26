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
 * ModuleName  : amf
 * File        : clAmsDebugCli.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains functions specific to AMS debug CLI.
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/

#ifndef _CL_AMS_DEBUG_CLI_H_
#define _CL_AMS_DEBUG_CLI_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
* Include files needed to compile this file
*****************************************************************************/

#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <string.h>
#include <clAmsMgmtClientApi.h>
#include <clAmsErrors.h>
#include <clBufferApi.h>

/*
 * Debug CLI related function declarations
 */

extern ClRcT
    clAmsDebugCliInitialization(void);

extern ClRcT
    clAmsDebugCliFinalization(void);

extern ClRcT
clAmsDebugCliEntityLockAssignment(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret,
        CL_IN  ClUint32T  retLen);

extern ClRcT
clAmsDebugCliEntityLockInstantiation(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret,
        CL_IN  ClUint32T  retLen);

extern ClRcT
clAmsDebugCliEntityUnlock(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret,
        CL_IN  ClUint32T  retLen);

extern ClRcT
clAmsDebugCliEntityShutdown(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret,
        CL_IN  ClUint32T  retLen);

extern ClRcT
clAmsDebugCliEntityRestart(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret,
        CL_IN  ClUint32T  retLen);

extern ClRcT
clAmsDebugCliEntityMarkRepaired(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret);

extern ClRcT  
clAmsDebugCliMakeEntityStruct(
        CL_OUT  ClAmsEntityT  *entity,
        CL_IN  ClCharT  *entityName,
        CL_IN  ClCharT  *entityType );

extern void 
clAmsDebugCliUsage(
        CL_OUT  ClCharT  *ret,
        CL_IN     ClUint32T retLen);

extern ClRcT   
clAmsDebugCliAdminAPI(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret);

extern ClRcT   
clAmsDebugCliSISwap(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret);

extern ClRcT   
clAmsDebugCliSGAdjust(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret);

extern ClRcT
clAmsDebugCliFaultReport(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret);

extern ClRcT   
clAmsDebugCliNodeJoin(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret);

extern ClRcT   
clAmsDebugCliPGTrackAdd(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret);

extern ClRcT   
clAmsDebugCliPGTrackStop(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret);

extern ClRcT   
clAmsDebugCliPGTrackDispatch(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret);

extern ClRcT   
clAmsDebugCliPrintAmsDB(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret);

extern ClRcT   
clAmsDebugCliPrintAmsDBXML(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret);
    
extern ClRcT   
clAmsDebugCliEntityPrint(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret);

extern ClRcT
clAmsDebugCliEntityDebugEnable(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret );

extern ClRcT
clAmsDebugCliEntityDebugDisable(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret );

extern ClRcT
clAmsDebugCliEntityDebugGet(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret );

extern ClRcT   
clAmsDebugCliDeXMLizeInvocation(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret);

extern ClRcT   
clAmsDebugCliDeXMLizeDB(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret);

extern ClRcT   
clAmsDebugCliXMLizeInvocation(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret);

extern ClRcT   
clAmsDebugCliXMLizeDB(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret);

extern ClRcT   
clAmsDebugCliCkptTest(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret);

extern ClRcT
clAmsDebugCliParseDebugFlagsStr(
        CL_IN  ClCharT  *debugFlagsStr,
        CL_OUT  ClUint8T  *debugFlags );

extern void 
clAmsDebugCliDebugCommandUsage(
        CL_OUT  ClCharT  *ret,
        CL_IN   ClUint32T retLen,
        CL_IN  ClUint32T  debugCommand );

extern ClRcT
clAmsDebugCliParseDebugFlags(
        CL_OUT  ClCharT  **debugFlagsStr,
        CL_IN  ClUint8T  debugFlags );

extern ClRcT   
clAmsDebugCliSCStateChange(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret);

extern ClRcT   
clAmsDebugCliEventTest(
        ClUint32T argc,
        ClCharT **argv,
        ClCharT** ret);

extern ClRcT
clAmsDebugCliEnableLogToConsole(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret );

extern ClRcT
clAmsDebugCliDisableLogToConsole(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret );


extern ClRcT
clAmsDebugCliEntityAlphaFactor(
        CL_IN  ClUint32T  argc,
        CL_IN  ClCharT  **argv,
        CL_OUT  ClCharT  **ret );

extern ClRcT
clAmsDebugCliEntityTrigger(
                           CL_IN  ClUint32T  argc,
                           CL_IN  ClCharT  **argv,
                           CL_OUT  ClCharT  **ret );
                               
extern ClRcT
clAmsDebugCliInitialization(void);

extern ClRcT
clAmsDebugCliFinalization(void);

#ifdef __cplusplus
}
#endif

#endif /* _CL_AMS_DEBUG_CLI_H_ */
