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
 * File        : clMsgGroupRR.c
 *******************************************************************************/

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clHash.h>
#include <clLogApi.h>
#include <clMsgGroupRR.h>

ClOsalMutexT gClGroupRRLock;

#define CL_MSG_GROUP_RR_BITS          (5)
#define CL_MSG_GROUP_RR_BUCKETS       (1 << CL_MSG_GROUP_RR_BITS)
#define CL_MSG_GROUP_RR_MASK          (CL_MSG_GROUP_RR_BUCKETS - 1)


static struct hashStruct *ppMsgQGroupHashTable[CL_MSG_GROUP_RR_BUCKETS];


static __inline__ ClUint32T clMsgGroupHash(const SaNameT *pQGroupName)
{
    return (ClUint32T)((ClUint32T)pQGroupName->value[0] & CL_MSG_GROUP_RR_MASK);
}


static ClRcT _clMsgGroupRRAdd(ClMsgGroupRoundRobinT *pGroupHashEntry)
{
    ClUint32T key = clMsgGroupHash(&pGroupHashEntry->name);
    return hashAdd(ppMsgQGroupHashTable, key, &pGroupHashEntry->hash);
}


static __inline__ void _clMsgGroupRRDel(ClMsgGroupRoundRobinT *pGroupHashEntry)
{
    hashDel(&pGroupHashEntry->hash);
}


ClBoolT clMsgGroupRRExists(const SaNameT *pQGroupName, ClMsgGroupRoundRobinT **ppGroupRR)
{
    register struct hashStruct *pTemp;
    ClUint32T key = clMsgGroupHash(pQGroupName);

    for(pTemp = ppMsgQGroupHashTable[key];  pTemp; pTemp = pTemp->pNext)
    {   
        ClMsgGroupRoundRobinT *pMsgQGroupEntry = hashEntry(pTemp, ClMsgGroupRoundRobinT, hash);
        if(pQGroupName->length == pMsgQGroupEntry->name.length && 
                memcmp(pMsgQGroupEntry->name.value, pQGroupName->value, pQGroupName->length) == 0)
        {
            if(ppGroupRR != NULL)
                *ppGroupRR = pMsgQGroupEntry;
            return CL_TRUE;
        } 
    }   
    return CL_FALSE;
}

ClRcT clMsgGroupRRAdd(SaNameT *pGroupName, ClUint32T rrIndex, ClMsgGroupRoundRobinT **ppGroupRR)
{
    ClRcT rc;
    ClMsgGroupRoundRobinT *pGroupRR;
    
    if(clMsgGroupRRExists(pGroupName, ppGroupRR) == CL_TRUE)
    {
        rc = CL_ERR_ALREADY_EXIST;
        clLogWarning("GRP", "ADD", "A group with name [%.*s] already exists. error code [0x%x].",pGroupName->length, pGroupName->value, rc);
        goto error_out;
    }

    pGroupRR = (ClMsgGroupRoundRobinT *)clHeapAllocate(sizeof(ClMsgGroupRoundRobinT));
    if(pGroupRR == NULL)
    {
        rc = CL_ERR_NO_MEMORY;
        clLogError("GRP", "ADD", "Failed to allocate %u bytes of memory. error code [0x%x].", 
                (ClUint32T) sizeof(ClMsgGroupRoundRobinT), rc);
        goto error_out;
    }

    memset(pGroupRR, 0, sizeof(ClMsgGroupRoundRobinT));

    saNameCopy(&pGroupRR->name, pGroupName);
    pGroupRR->rrIndex = rrIndex;

    rc = _clMsgGroupRRAdd(pGroupRR);
    if(rc != CL_OK)
    {
        clLogError("GRP", "ADD", "Failed to add entry to the group hash table. error code [0x%x].", rc);
        goto error_out;
    }

    if(ppGroupRR != NULL)
        *ppGroupRR = pGroupRR;

error_out:
    return rc;
}

ClRcT clMsgGroupRRDelete(SaNameT *pGroupName)
{
    ClRcT rc = CL_OK;
    ClMsgGroupRoundRobinT *pGroupRR;

    if(clMsgGroupRRExists(pGroupName, &pGroupRR) == CL_FALSE)
    {
        rc = CL_ERR_DOESNT_EXIST;
        clLogWarning("GRP", "DEL", "Group [%.*s] doesnot exist. error code [0x%x]", pGroupName->length, pGroupName->value, rc);
        goto error_out;
    }

    _clMsgGroupRRDel(pGroupRR);

    clHeapFree(pGroupRR);

error_out:
    return rc;
}
