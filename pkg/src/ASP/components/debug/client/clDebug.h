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
 * ModuleName  : debug
 * File        : clDebug.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#ifndef _DEBUG_H_

#include <clDebugApi.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ARGS		20
#define MAX_ARG_BUF_LEN		128

#define CL_DEBUG_CLI_PROMPT_LEN        40
#define CL_DEBUG_CLI_USER_PROMPT_LEN   CL_DEBUG_CLI_PROMPT_LEN - 4 - 14 - CL_DEBUG_COMP_PROMPT_LEN

typedef struct
{
    /* 
     * A List of commands and their respective functions and help
     */
    ClDebugFuncEntryT  *pFuncDescList;

    /*
     * Num of functions in this group
     */
    ClUint32T           numFunc;
}ClDebugFuncGroupT;


/**
 * This is the debug Object that is present along with the EO.
 */
typedef struct ClDebugObjT
{
/**
 * Handle Database which holds list of all commands and their respective functions and help.
 */
    ClHandleDatabaseHandleT  hDebugFnDB;
    
    /*
     * Debug response context key.
     */
    ClUint32T          debugTaskKey;
/**
 * Number of commands provided.
 */
    ClUint32T      numFunc;

/**
 * Name of the component.
 */
    ClCharT        compName[CL_DEBUG_COMP_NAME_LEN];

/**
 * Prompt for the component.
 */
    ClCharT        compPrompt[CL_DEBUG_COMP_PROMPT_LEN];
} ClDebugObjT;

typedef struct
{
  ClCharT            *pCommandName;
  ClDebugFuncEntryT  *pFuncEntry;
} ClDebugInvokeCookieT;

ClRcT clDebugClientTableRegister(ClEoExecutionObjT *pThis);

#ifdef __cplusplus
}
#endif

#endif

