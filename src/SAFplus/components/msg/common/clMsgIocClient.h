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
 * ModuleName  : msg                                                          
 * File        : clMsgIocClient.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 *   This file contains IPIs that support MSG sending via IOC
 *
 *****************************************************************************/

#ifndef __CL_MSG_IOC_CLIENT_H__
#define	__CL_MSG_IOC_CLIENT_H__

#include <saMsg.h>
#include <clMsgCommon.h>

#ifdef	__cplusplus
extern "C" {
#endif
    
typedef void (*MsgIocMessageSendAsyncCallbackT) (CL_IN ClRcT rc, CL_IN void* pCookie);

ClRcT clMsgIocSendSync(ClIocAddressT * pDestAddr,
                     ClMsgMessageSendTypeT sendType,
                     const ClNameT *pDestination,
                     const SaMsgMessageT *pMessage,
                     ClInt64T sendTime,
                     ClHandleT senderHandle,
                     ClInt64T timeout
                    );


ClRcT clMsgIocSendAsync(ClIocAddressT * pDestAddr,
                     ClMsgMessageSendTypeT sendType,
                     const ClNameT *pDestination,
                     const SaMsgMessageT *pMessage,
                     ClInt64T sendTime,
                     ClHandleT senderHandle,
                     ClInt64T timeout,
                     MsgIocMessageSendAsyncCallbackT fpAsyncCallback,
                     void *cookie
                    );

ClRcT clMsgIocCltInitialize(void);
ClRcT clMsgIocCltFinalize(void);

#ifdef	__cplusplus
}
#endif

#endif	/* __CL_MSG_IOC_CLIENT_H__ */

