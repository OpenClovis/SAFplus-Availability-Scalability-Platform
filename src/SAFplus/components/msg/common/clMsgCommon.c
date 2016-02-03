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

ClBoolT gClMsgInit = CL_FALSE;
ClIocNodeAddressT gLocalAddress;
ClIocPortT gLocalPortId;

ClRcT clMsgIovecToMessageCopy(SaMsgMessageT **ppNewMsg, ClMsgMessageIovecT *pMessage, ClUint32T index)
{
    ClRcT rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
    ClNameT *pTempName = NULL;
    SaMsgMessageT *pTempMessage;

    pTempMessage = (SaMsgMessageT *)clHeapAllocate(sizeof(SaMsgMessageT));
    if(pTempMessage == NULL)
    {
        clLogError("MSG", "ALLOC", "Failed to allocate memory of %zd bytes. error code [0x%x].", sizeof(SaMsgMessageT), rc);
        goto error_out;
    }

    if(pMessage->senderName != NULL)
    {
        pTempName = (ClNameT *)clHeapAllocate(sizeof(ClNameT));
        if(pTempName == NULL)
        {
            clLogError("MSG", "ALLOC", "Failed to allocate memory of %zd bytes. error code [0x%x].", sizeof(ClNameT), rc);
            goto error_out_1;
        }
    }

    pTempMessage->type = pMessage->type;
    pTempMessage->version = pMessage->version;
    pTempMessage->priority = pMessage->priority;
    if(pMessage->senderName != NULL)
        memcpy(pTempName, pMessage->senderName, sizeof(ClNameT));
    pTempMessage->senderName = (SaNameT*)pTempName;
    pTempMessage->data = pMessage->pIovec[index].iov_base;
    pTempMessage->size = pMessage->pIovec[index].iov_len;
    pMessage->pIovec[index].iov_base = NULL;
    *ppNewMsg = pTempMessage;

    rc = CL_OK;
    goto out;

error_out_1:
    clHeapFree(pTempMessage);
error_out:
out:
    return rc;
}

ClRcT clMsgIovecToIovecCopy(ClMsgMessageIovecT **ppNewMsg, ClMsgMessageIovecT *pMessage)
{
    ClMsgMessageIovecT *pTempMessage;
    ClRcT rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
    ClNameT *pTempName = NULL;
    iovec_t *pTempIovec = NULL;
    ClUint8T *iov_base = NULL;
    ClUint32T i;

    pTempMessage = (ClMsgMessageIovecT *)clHeapAllocate(sizeof(ClMsgMessageIovecT));

    if(pTempMessage == NULL)
    {
        clLogError("MSG", "ALLOC", "Failed to allocate memory of %zd bytes. error code [0x%x].", sizeof(ClMsgMessageIovecT), rc);
        goto error_out;
    }

    if(pMessage->senderName != NULL)
    {
        pTempName = (ClNameT *)clHeapAllocate(sizeof(ClNameT));
        if(pTempName == NULL)
        {
            clLogError("MSG", "ALLOC", "Failed to allocate memory of %zd bytes. error code [0x%x].", sizeof(ClNameT), rc);
            goto error_out_1;
        }
        memcpy(pTempName, pMessage->senderName, sizeof(ClNameT));
    }

    if(pMessage->pIovec != NULL)
    {
        pTempIovec = (iovec_t *)clHeapAllocate(pMessage->numIovecs * sizeof(iovec_t));
        if(pTempIovec == NULL)
        {
            clLogError("MSG", "ALLOC", "Failed to allocate memory of %zd bytes. error code [0x%x].", pMessage->numIovecs * sizeof(iovec_t), rc);
            goto error_out_2;
        }
    }

    for (i = 0; i < pMessage->numIovecs; i++)
    {
        if (pMessage->pIovec[i].iov_base)
        {
            iov_base = NULL;
            iov_base = (ClUint8T *)clHeapAllocate(pMessage->pIovec[i].iov_len);
            if(iov_base == NULL)
            {
                clLogError("MSG", "ALLOC", "Failed to allocate memory of %zd bytes. error code [0x%x].", pMessage->pIovec[i].iov_len, rc);
                goto error_out_3;
            }
            memcpy(iov_base, pMessage->pIovec[i].iov_base, pMessage->pIovec[i].iov_len);
            pTempIovec[i].iov_len = pMessage->pIovec[i].iov_len;
            pTempIovec[i].iov_base = iov_base;
        }
    }

    pTempMessage->type = pMessage->type;
    pTempMessage->version = pMessage->version;
    pTempMessage->priority = pMessage->priority;
    pTempMessage->senderName = (SaNameT*)pTempName;
    pTempMessage->numIovecs = pMessage->numIovecs;
    pTempMessage->pIovec = pTempIovec;
    *ppNewMsg = pTempMessage;

    rc = CL_OK;
    goto out;

error_out_3:
    for (i = 0; i < pMessage->numIovecs; i++)
    {
        if (pTempIovec[i].iov_base)
            clHeapFree(pTempIovec[i].iov_base);
    }
    clHeapFree(pTempIovec);
error_out_2:
    clHeapFree(pTempName);
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
