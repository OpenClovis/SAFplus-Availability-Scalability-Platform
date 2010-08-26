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
 * File        : clDebugLog.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains log messages 
 *
 *
 *****************************************************************************/
#ifndef _CL_DEBUG_LOG_H_
#define _CL_DEBUG_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clLogApi.h>
    
#define CL_DEBUG_LIB_CLIENT   "debug_client"
extern ClCharT     *clDebugLogMsg[];

#define CL_DEBUG_LOG_1_CMD_SEND			clDebugLogMsg[0]
#define CL_DEBUG_LOG_1_CONTEXT_SET		clDebugLogMsg[1]


#ifdef __cplusplus
}
#endif


#endif /* _CL_DEBUG_LOG_H_ */
