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
 * ModuleName  : gms
 * File        : clGmsCli.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Contains data structures and function prototypes for clGmsCli.c
 *
 *****************************************************************************/

#ifndef _CL_GMS_CLI_H_
#define _CL_GMS_CLI_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <clCommon.h>
#include <clDebugApi.h>

#define CL_GMS_PRINT_STR_LENGTH     1024
#define GMS_TOTAL_CLI_COMMANDS      6

extern ClDebugFuncEntryT gmsCliCommandsList[GMS_TOTAL_CLI_COMMANDS];

extern void _clGmsCliPrint (char **retstr, char *format , ... );

#define GMS_COMMAND_PROMPT  "gms"

    
#ifdef __cplusplus
}
#endif

#endif /* _CL_GMS_CLI_H_ */
