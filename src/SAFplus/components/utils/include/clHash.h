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
#ifndef _CL_HASH_H_
#define _CL_HASH_H_

#include <string.h>

#include <clCommon.h>
#include <clCommonErrors.h>

struct hashStruct
{
    struct hashStruct *pNext;
    struct hashStruct **ppPrev;
};

static __inline__ ClRcT hashAdd
(struct hashStruct **ppHashTable,ClUint32T key,struct hashStruct *pEntry)
{
    ClRcT rc;
    struct hashStruct **ppTable;
    ppTable = ppHashTable + key;
    if( (pEntry->pNext = *ppTable) ) {
        pEntry->pNext->ppPrev = &pEntry->pNext;
    } 
    *(pEntry->ppPrev = ppTable) = pEntry;
    rc = CL_OK;
    return rc;
}

static __inline__ ClRcT hashDel(struct hashStruct *pEntry)
{
    ClRcT rc = CL_ERR_INVALID_PARAMETER;
    if(pEntry->ppPrev) 
    {
        rc = CL_OK;
        if(pEntry->pNext)
        {
            pEntry->pNext->ppPrev = pEntry->ppPrev;
        }
        *pEntry->ppPrev = pEntry->pNext;
        pEntry->pNext = NULL,pEntry->ppPrev = NULL;
    }
    return rc;
}

#define hashEntry(element,cast,field) \
(cast *) ( (ClUint8T*)(element) - (ClWordT) (&((cast*)0)->field) )

#endif
