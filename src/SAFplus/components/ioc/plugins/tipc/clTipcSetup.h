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
#ifndef __CL_TIPC_SETUP_H__
#define __CL_TIPC_SETUP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <clIocConfig.h>
#include <clIocSetup.h>

#define CL_TIPC_SET_TYPE(v) clTipcSetType(v,CL_TRUE)

#define CL_TIPC_BASE_TYPE         (0x100) /* Type codes lower than this are off use for ASP */

/* The main thread waittime is in seconds */
#define CL_TIPC_TOP_SRV_TIMEOUT   (1000)
#define CL_TIPC_TOP_SRV_READY_TIMEOUT (200)

#define CL_TIPC_HIGH_PRIORITY    TIPC_CRITICAL_IMPORTANCE
#define CL_TIPC_DEFAULT_PRIORITY TIPC_LOW_IMPORTANCE

extern ClUint32T clTipcSetType(ClUint32T portId,ClBoolT setFlag);
extern ClRcT clTipcFdGet(ClIocPortT port, ClInt32T *fd);
extern ClRcT clTipcDoesNodeAlreadyExist(void);
extern ClInt32T gClTipcXportId;
extern ClCharT  gClTipcXportType[CL_MAX_NAME_LENGTH];

#ifdef __cplusplus
   }
#endif
#endif
