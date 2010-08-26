/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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
 * ModuleName  : message
 * File        : clMsgDebugCli.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This header file contains functions exposed by message
 *          service debug cli.
 *****************************************************************************/


#ifndef __CL_MSG_DEBUG_CLI_H__
#define __CL_MSG_DEBUG_CLI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCommonErrors.h>

#include <clDebugApi.h>
#include <clHandleApi.h>

#include <clHandleIpi.h>
#include <clMsgQueue.h>
#include <clMsgDebugInternal.h>    

extern ClRcT clMsgDebugCliRegister(void);

extern ClRcT clMsgDebugCliDeregister(void);
    
#ifdef __cplusplus
}
#endif


#endif
