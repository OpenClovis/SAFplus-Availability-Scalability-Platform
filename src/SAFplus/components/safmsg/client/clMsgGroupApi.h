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
 *//*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : message
 * File        : clMsgEo.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This module contains all the house keeping functionality
 *          for MS - EOnization & association with CPM, debug, etc.
 *****************************************************************************/


#ifndef __CL_MSG_GROUP_API_H__
#define __CL_MSG_GROUP_API_H__

#include <clCommon.h>
#include <saAis.h>
#include <saMsg.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    SaNameT groupName;
    SaMsgQueueGroupNotificationBufferT *pNotificationBuffer;
    SaAisErrorT rc;
}ClMsgAppQGroupTrackCallbakParamsT;

ClRcT clMsgClientsTrackCallback_4_0_0(SaMsgHandleT clientHandle, SaNameT *pGroupName, SaMsgQueueGroupNotificationBufferT *pData);
#ifdef __cplusplus
}
#endif


#endif
