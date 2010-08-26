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
 * File        : clAmsMgmtServerRmd.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the server side implementation of the AMS management
 * API.
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/

#ifndef _CL_AMS_MGMT_SERVER_RMD_H_
#define _CL_AMS_MGMT_SERVER_RMD_H_

# ifdef __cplusplus
extern "C"
{
# endif

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <clCommon.h>
#include <clBufferApi.h>
#include <clAmsMgmtCommon.h>

/************************************************************************
Function decalration for Management API RMD related functions 
************************************************************************/

extern ClRcT 
_clAmsMgmtReadInputBuffer(
       CL_IN  ClBufferHandleT  in,
       CL_OUT  ClUint8T  *inBuf,
       CL_IN  ClUint32T  reqInLen );


extern  ClRcT 
_clAmsMgmtReadInputBufferEntitySetConfig(
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClUint8T  *inBuf );

#ifdef POST_RC2

extern  ClRcT 
_clAmsMgmtWriteGetEntityListBuffer(
        CL_OUT  ClBufferHandleT  out,
        CL_IN  clAmsMgmtEntityGetEntityListResponseT  *res );

extern ClRcT
_clAmsMgmtWriteOutputBufferFindEntityByName (
        CL_OUT  ClBufferHandleT  out,
        CL_IN  ClAmsEntityRefT  *entityRef );
#endif

# ifdef __cplusplus
}
# endif

#endif /* _CL_AMS_MGMT_SERVER_RMD_H_ */
