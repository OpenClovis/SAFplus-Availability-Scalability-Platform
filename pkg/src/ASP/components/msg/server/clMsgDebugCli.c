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
 * ModuleName  : message
 * File        : clMsgDebugCli.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This file contains debug cli commands for message service.
 *****************************************************************************/

#include <clMsgDebugCli.h>

static ClRcT clMsgQueueInfoCb(ClHandleDatabaseHandleT hdlDb,
                              ClHandleT hdl,
                              ClPtrT pCookie)
{
    ClRcT rc = CL_OK;
    ClMsgQueueInfoT *pQInfo = NULL;
    ClDebugPrintHandleT dbgHdl = (ClDebugPrintHandleT)pCookie;
    ClNameT *qName = NULL;
    ClUint32T i = 0;

    rc = clHandleCheckout(hdlDb, hdl, (void **)&pQInfo);
    if (rc != CL_OK)
    {
        clLogError("MSG", "CLI", "Handle checkout failed, error [%#x]", rc);
        goto out;
    }

    CL_OSAL_MUTEX_LOCK(&(pQInfo->qLock));
    
    qName = &(pQInfo->pQHashEntry->qName);    
    clDebugPrint(dbgHdl, "Name : [%.*s]\n", qName->length, qName->value);

    clDebugPrint(dbgHdl, "Open flags : ");
    if (pQInfo->openFlags & SA_MSG_QUEUE_CREATE)
    {
        clDebugPrint(dbgHdl, "CREATE");
    }
    if (pQInfo->openFlags & SA_MSG_QUEUE_RECEIVE_CALLBACK)
    {
        clDebugPrint(dbgHdl, " | RECEIVE_CALLBACK");
    }
    if (pQInfo->openFlags & SA_MSG_QUEUE_EMPTY)
    {
        clDebugPrint(dbgHdl, " | EMPTY");
    }
    clDebugPrint(dbgHdl, "\n");
    
    clDebugPrint(dbgHdl, "Persistent : %s\n",
                 (pQInfo->creationFlags & SA_MSG_QUEUE_PERSISTENT) ?
                 "Yes":
                 "No");

    if (pQInfo->creationFlags & SA_MSG_QUEUE_PERSISTENT)
    {
        clDebugPrint(dbgHdl, "Retention time : -\n");
    }
    else
    {
        clDebugPrint(dbgHdl,
                     "Retention time : %lld\n",
                     pQInfo->retentionTime);
    }
    
    clDebugPrint(dbgHdl,
                 "%5s %10s %10s %10s %10s\n",
                 "Priority",
                 " Size (bytes)",
                 " Used (bytes)",
                 " Avail (bytes)",
                 " Number of messages");
    for (i = 0; i < CL_MSG_QUEUE_PRIORITIES; ++i)
    {
        clDebugPrint(dbgHdl,
                     "%3d %15lld %15lld %10lld %15d\n",
                     i,
                     pQInfo->size[i],
                     pQInfo->usedSize[i],
                     (pQInfo->size[i]-pQInfo->usedSize[i]),
                     pQInfo->numberOfMessages[i]);
    }

    clDebugPrint(dbgHdl, "State : %s",
                 (pQInfo->state == CL_MSG_QUEUE_CREATED) ? "CREATED" :
                 (pQInfo->state == CL_MSG_QUEUE_OPEN) ? "OPEN" :
                 (pQInfo->state == CL_MSG_QUEUE_CLOSED) ? "CLOSED" :
                 "INVALID STATE");

    clDebugPrint(dbgHdl, "\n");
    
    CL_OSAL_MUTEX_UNLOCK(&(pQInfo->qLock));

    return CL_OK;
out:
    return rc;
}

static ClRcT clMsgDebugCliQueueInfo(ClUint32T argc,
                                    ClCharT **argv,
                                    ClCharT **ret)
{
    ClRcT rc = CL_OK;
    ClDebugPrintHandleT dbgHdl = 0;

    clDebugPrintInitialize(&dbgHdl);
    
    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    rc = clHandleWalk(gClMsgQDatabase, clMsgQueueInfoCb, (ClPtrT)dbgHdl);
    if (rc != CL_OK)
    {
        clLogError("MSG", "CLI", "Handle walk failed, error [%#x]", rc);
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        goto out;
    }
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

    clDebugPrintFinalize(&dbgHdl, ret);
    
    return CL_OK;
out:    
    return rc;
}

static ClDebugFuncEntryT msgDebugFuncList[] =
{
    {
        clMsgDebugCliQueueInfo,
        "msgQueueInfo",
        "Display information about the message queues"
    },

    {
        NULL,
        "",
        ""
    }
};

static ClHandleT gMsgDebugHdl;

ClRcT clMsgDebugCliRegister(void)
{
    ClRcT rc = CL_OK;

    rc = clDebugPromptSet("MSG");
    if (rc != CL_OK)
    {
        clLogError("MSG", "CLI",
                   "Unable to set debug prompt, error [%#x]", rc);
        goto out;
    }

    rc = clDebugRegister(msgDebugFuncList,
                         sizeof(msgDebugFuncList)/sizeof(msgDebugFuncList[0]),
                         &gMsgDebugHdl);
    if (rc != CL_OK)
    {
        clLogError("MSG", "CLI",
                   "Unable to register debug functions, error [%#x]", rc);
        goto out;
    }

    return CL_OK;
out:
    return rc;
}

ClRcT clMsgDebugCliDeregister(void)
{
    return clDebugDeregister(gMsgDebugHdl);
}
