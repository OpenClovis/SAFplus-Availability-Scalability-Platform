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
 * File        : clMsgCommon.c
 *******************************************************************************/

/*******************************************************************************
 * Description : This file contains function which are needed to both messaging 
 *               client and server.
 *****************************************************************************/


#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clHeapApi.h>
#include <clTimerApi.h>
#include <clLogApi.h>
#include <saMsg.h>
#include <clMsgCommon.h>



ClRcT clMsgMessageToMessageCopy(SaMsgMessageT **ppNewMsg, SaMsgMessageT *pMessage)
{
    ClRcT rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
    ClNameT *pTempName = NULL;
    ClUint8T *pTempData = NULL;
    SaMsgMessageT *pTempMessage;

    pTempMessage = (SaMsgMessageT *)clHeapAllocate(sizeof(SaMsgMessageT));
    if(pTempMessage == NULL)
    {
        clLogError("MSG", "ALLOC", "Failed to allcoate memory of %zd bytes. error code [0x%x].", sizeof(SaMsgMessageT), rc);
        goto error_out;
    }

    if(pMessage->senderName != NULL)
    {
        pTempName = (ClNameT *)clHeapAllocate(sizeof(ClNameT));
        if(pTempName == NULL)
        {
            clLogError("MSG", "ALLOC", "Failed to allcoate memory of %zd bytes. error code [0x%x].", sizeof(ClNameT), rc);
            goto error_out_1;
        }
    }

    if(pMessage->data != NULL)
    {
        pTempData = (ClUint8T*)clHeapAllocate(pMessage->size);
        if(pTempData == NULL)
        {
            clLogError("MSG", "ALLOC", "Failed to allcoate memory of %llu bytes. error code [0x%x].", pMessage->size, rc);
            goto error_out_2;
        }
    }

    memcpy(pTempMessage, pMessage, sizeof(SaMsgMessageT));
    if(pMessage->senderName != NULL)
        memcpy(pTempName, pMessage->senderName, sizeof(ClNameT));
    if(pMessage->data != NULL)
        memcpy(pTempData, pMessage->data, pMessage->size);
    pTempMessage->senderName = (SaNameT*)pTempName;
    pTempMessage->data = pTempData;
    *ppNewMsg = pTempMessage;
    
    rc = CL_OK;
    goto out;
    
error_out_2:
    clHeapFree((pTempMessage)->senderName);
error_out_1:
    clHeapFree(pTempMessage);
error_out:
out:
    return rc;
}


void clMsgMessageFree(SaMsgMessageT *pMessage)
{
    if(pMessage)
    {
        if(pMessage->data !=NULL)
            clHeapFree(pMessage->data);
        if(pMessage->senderName !=NULL)
            clHeapFree(pMessage->senderName);
        clHeapFree(pMessage);
    }
}
