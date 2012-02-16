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
 * ModuleName  : message
 * File        : clMsgQCkptClient.h
 *******************************************************************************/

#ifndef __CL_MSG_CKPT_CLIENT_H__
#define	__CL_MSG_CKPT_CLIENT_H__

#include <clMsgCkptData.h>

#ifdef	__cplusplus
extern "C" {
#endif

extern ClCachedCkptClientSvcInfoT gMsgQCkptClient;
extern ClCachedCkptClientSvcInfoT gMsgQGroupCkptClient;
    
ClRcT clMsgQCkptInitialize(void);
ClRcT clMsgQCkptFinalize(void);
ClBoolT clMsgQCkptExists(const ClNameT *pQName, ClMsgQueueCkptDataT *pQueueData);
ClBoolT clMsgQGroupCkptExists(const ClNameT *pQGroupName, ClMsgQGroupCkptDataT *pQGroupData);
ClRcT clMsgQGroupCkptDataGet(const ClNameT *pQGroupName, ClMsgQGroupCkptDataT *pQGroupData);

#ifdef	__cplusplus
}
#endif

#endif	/* __CL_MSG_CKPT_CLIENT_H__ */

