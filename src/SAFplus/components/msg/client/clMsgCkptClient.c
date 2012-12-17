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
 * File        : clMsgQCkptClient.c
 *******************************************************************************/
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clLogApi.h>
#include <clMsgCkptClient.h>

/* 
 * In SAI-AIS-MSG-B.03.01.pdf section 3.1.2, 
 * "Message queues and message queue groups have distinct name spaces" 
 */
ClCachedCkptClientSvcInfoT gMsgQCkptClient;
ClCachedCkptClientSvcInfoT gMsgQGroupCkptClient;

ClRcT clMsgQCkptInitialize(void)
{
    ClRcT rc = CL_OK;
    ClRcT retCode;

    const ClNameT msgQueueCkptName  = {
                     sizeof("CL_MsgQueueCkpt") - 1,
                     "CL_MsgQueueCkpt"
                    };
    rc = clCachedCkptClientInitialize(&gMsgQCkptClient, &msgQueueCkptName, CL_MSG_QUEUE_CKPT_SIZE);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize cached checkpoint client service for MSG queue: error code [0x%x].", rc);
        goto out;
    }

    const ClNameT msgQueueGroupCkptName  = {
                     sizeof("CL_MsgQueueGroupCkpt") - 1,
                     "CL_MsgQueueGroupCkpt"
                    };

    rc = clCachedCkptClientInitialize(&gMsgQGroupCkptClient, &msgQueueGroupCkptName, CL_MSG_QGROUP_CKPT_SIZE);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize cached checkpoint client service for MSG queue group: error code [0x%x].", rc);
        goto error_out1;
    }

    goto out;
error_out1:
    retCode = clCachedCkptClientFinalize(&gMsgQCkptClient);
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to finalize the CachedCkpt for MSG queue group. error code [0x%x].", retCode);
out:
    return rc;
}

ClRcT clMsgQCkptFinalize(void)
{
    ClRcT rc = CL_OK; 
    rc = clCachedCkptClientFinalize(&gMsgQCkptClient);
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to Finalize the CachedCkpt for MSG queue. error code [0x%x].", rc);

    rc = clCachedCkptClientFinalize(&gMsgQGroupCkptClient);
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to Finalize the CachedCkpt for MSG queue group. error code [0x%x].", rc);
    return rc;
}

ClBoolT clMsgQCkptExists(const ClNameT *pQName, ClMsgQueueCkptDataT *pQueueData)
{
    ClCachedCkptDataT *sectionData = NULL;

    clOsalSemLock(gMsgQCkptClient.cacheSem);
    clCachedCkptClientLookup(&gMsgQCkptClient, pQName, &sectionData);
    if (sectionData == NULL)
    {
        clOsalSemUnlock(gMsgQCkptClient.cacheSem);
        return CL_FALSE;
    }

    clMsgQueueCkptDataUnmarshal(pQueueData, sectionData);

    clOsalSemUnlock(gMsgQCkptClient.cacheSem);

    return CL_TRUE;

}

ClBoolT clMsgQGroupCkptExists(const ClNameT *pQGroupName, ClMsgQGroupCkptDataT *pQGroupData)
{
    ClCachedCkptDataT *sectionData = NULL;

    clOsalSemLock(gMsgQGroupCkptClient.cacheSem);
    clCachedCkptClientLookup(&gMsgQGroupCkptClient, pQGroupName, &sectionData);
    if (sectionData == NULL)
    {
        clOsalSemUnlock(gMsgQGroupCkptClient.cacheSem);
        return CL_FALSE;
    }

    clMsgQGroupCkptHeaderUnmarshal(pQGroupData, sectionData);

    clOsalSemUnlock(gMsgQGroupCkptClient.cacheSem);

    return CL_TRUE;
}

ClRcT clMsgQGroupCkptDataGet(const ClNameT *pQGroupName, ClMsgQGroupCkptDataT *pQGroupData)
{
    ClRcT rc = CL_OK;
    ClCachedCkptDataT *sectionData = NULL;

    clOsalSemLock(gMsgQGroupCkptClient.cacheSem);
    clCachedCkptClientLookup(&gMsgQGroupCkptClient, pQGroupName, &sectionData);
    if (sectionData == NULL)
    {
        clOsalSemUnlock(gMsgQGroupCkptClient.cacheSem);
        rc = CL_ERR_DOESNT_EXIST;
        return rc;
    }

    rc = clMsgQGroupCkptDataUnmarshal(pQGroupData, sectionData);

    clOsalSemUnlock(gMsgQGroupCkptClient.cacheSem);

    return rc;
}
