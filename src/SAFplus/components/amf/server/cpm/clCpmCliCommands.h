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
 * ModuleName  : amf
 * File        : clCpmCliCommands.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *      This header file contains the debug CLI related functions and 
 *  defines.
 *
 *
 *****************************************************************************/

#ifndef _CL_CPM_CLI_COMMANDS_H_
#define _CL_CPM_CLI_COMMANDS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define ONE_ARGUMENT        1
#define TWO_ARGUMENT        2
#define THREE_ARGUMENT      3
#define FOUR_ARGUMENT       4
#define FIVE_ARGUMENT       5
#define SIX_ARGUMENT        6

extern ClRcT cliEOListShow(ClUint32T argc, ClCharT *argv[], ClCharT **retStr);

extern ClRcT clCpmComponentListAll(ClUint32T argc,
                                   ClCharT *argv[],
                                   ClCharT **retStr);

extern ClRcT clCpmSUListAll(ClUint32T argc,
                            ClCharT *argv[],
                            ClCharT **retStr);
    
extern ClRcT clCpmClusterListAll(ClUint32T argc,
                                 ClCharT *argv[],
                                 ClCharT **retStr);
    
extern ClRcT clCpmComponentReport(ClUint32T argc,
                                  ClCharT **argv,
                                  ClCharT **retStr);
    
extern ClRcT clCpmCompGet(ClUint32T argc,
                          ClCharT *argv[],
                          ClCharT **retStr);
    
extern ClRcT clCpmShutDown(ClUint32T argc,
                           ClCharT **argv,
                           ClCharT **retStr);

extern ClRcT clCpmMiddlewareRestartCommand(ClUint32T argc,
                                           ClCharT **argv,
                                           ClCharT **retStr);

extern ClRcT clCpmShutDown(ClUint32T argc,
                           ClCharT **argv,
                           ClCharT **retStr);

extern ClRcT clCpmNodeErrorReport(ClUint32T argc, ClCharT **argv, ClCharT **result);

extern ClRcT clCpmNodeErrorClear(ClUint32T argc, ClCharT **argv, ClCharT **result);

extern ClRcT clCpmCliNodeDelete(ClUint32T argc, ClCharT **argv, ClCharT **result);

extern ClRcT clCpmRestart(ClUint32T argc,
                          ClCharT **argv,
                          ClCharT **retStr);
    
extern ClRcT clCpmNodeNameGet(ClUint32T argc,ClCharT **argv,ClCharT **retStr);

extern ClRcT clCpmHeartbeat(ClUint32T argc, ClCharT **argv, ClCharT **retStr);

extern ClRcT clCpmLogFileRotate(ClUint32T argc, ClCharT **argv, ClCharT **retStr);

extern ClRcT clCpmUptimeGet(ClUint32T argc,
                            ClCharT **argv,
                            ClCharT **retStr);

# ifdef __cplusplus
}
# endif

#endif                          /* _CL_CPM_CLI_COMMANDS_H_ */
