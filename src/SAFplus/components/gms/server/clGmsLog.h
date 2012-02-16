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
 * File        : clGmsLog.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This private header contains the logmessages which are logged by the GMS
 * server instance. 
 *****************************************************************************/
#ifndef _CL_GMS_LOG_H_
#define _CL_GMS_LOG_H_

#ifdef __cplusplus 
extern "C" {
#endif 

extern ClCharT clGmsLogMsg[9][256];

#define     CL_GMS_SERVER_STARTED               clGmsLogMsg[0]
#define     CL_GMS_SERVER_TERMINATING           clGmsLogMsg[1]
#define     CL_GMS_SERVER_BOOTUP_FAILED         clGmsLogMsg[2]

#define     CL_GMS_NEW_GROUP_CREATED            clGmsLogMsg[3]
#define     CL_GMS_GROUP_DESTROYED              clGmsLogMsg[4]
#define     CL_GMS_GROUP_MEMBER_JOINED          clGmsLogMsg[5]
#define     CL_GMS_GROUP_MEMBER_LEFT            clGmsLogMsg[6]

#define     CL_GMS_CHANGE_OF_LEADERSHIP         clGmsLogMsg[7]
#define     CL_GMS_GROUP_RECONFIGURATION        clGmsLogMsg[8]


# ifdef __cplusplus
}
# endif

#endif							/* _CL_GMS_LOG_H_ */ 
