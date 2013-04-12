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
 * File        : clMsgSrvSender.c
 *******************************************************************************/
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clMsgCommon.h>
#include <clLogApi.h>
#include <clXdrApi.h>

ClRcT clMsgMessageReceiveCallback(SaMsgQueueHandleT qHandle)
{
    /* MSG queue instance located on MSG server does not send notification after it receives message. */
    ClRcT rc = CL_ERR_BAD_OPERATION;
    clLogError("MSG", "RCVcb", "MSG queue instance located on MSG server does not send notification after it receives message. error code [0x%x].", rc);
    return rc;
}

ClRcT clMsgReplyReceived(ClMsgMessageIovecT *pMessage, SaTimeT sendTime, SaMsgSenderIdT senderHandle, SaTimeT timeout)
{
    /* MSG queue instance located on MSG server does not receive reply message */
    ClRcT rc = CL_ERR_BAD_OPERATION;
    clLogError("MSG", "GOT-REPLY", "MSG queue instance located on MSG server does not receive reply message. error code [0x%x].", rc);
    return rc;
}
