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
#ifndef WITH_PURIFY
#error "WITH_PURIFY undefined in clBufferPurify.c"
#endif

#ifndef _CL_BUFFER_C_
#error "_CL_BUFFER_C_ undefined. clBufferPurify.c should be only included from clBuffer.c"
#endif

static ClRcT
clBufferFromPoolAllocatePurify(ClPoolT pool, 
                           ClUint32T actualLength, 
                           ClUint8T **ppChunk,
                           void **ppCookie)
{
    ClRcT rc = CL_RC(CL_CID_BUFFER,CL_ERR_NO_MEMORY);
    void *pAddress;
    pAddress = malloc(actualLength);
    if(pAddress)
    {
        rc = CL_OK;
        *ppChunk = pAddress;
    }
    return rc;
}

static ClRcT clBufferFromPoolFreePurify
(ClUint8T *pChunk,ClUint32T size,void *pCookie)
{
    free(pChunk);
    return CL_OK;
}

static void clBufferModeSet(void)
{
    gClBufferFromPoolAllocate = clBufferFromPoolAllocatePurify;
    gClBufferFromPoolFree = clBufferFromPoolFreePurify;
    gClBufferLengthCheck = clBufferLengthCheck;
    if(gBufferDebugLevel > 0 )
    {
        gClBufferLengthCheck = clBufferLengthCheckDebug;
    }
}

static void clBufferModeUnset(void)
{
    gClBufferFromPoolAllocate = NULL;
    gClBufferFromPoolFree = NULL;
}

static ClRcT
clBufferPoolInitialize(const ClBufferPoolConfigT *pBufferPoolConfigUser)
{
    ClRcT rc = CL_OK;
    ClBufferPoolConfigT *pBufferPoolConfig = &gBufferManagementInfo.bufferPoolConfig;

    rc = CL_BUFFER_RC(CL_ERR_NO_MEMORY);

    memcpy(pBufferPoolConfig,pBufferPoolConfigUser,sizeof(*pBufferPoolConfig));

    if(! (pBufferPoolConfig->pPoolConfig = clHeapAllocate(sizeof(*pBufferPoolConfig->pPoolConfig) * pBufferPoolConfigUser->numPools)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error allocating memory\n"));
        goto out;
    }

    /*Just allocate the handles which wont be used*/
    if(!(gBufferManagementInfo.pPoolHandles = clHeapAllocate(sizeof(ClPoolT) * pBufferPoolConfigUser->numPools)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error allocating memory\n"));
        goto out_free;
    }

    memset(gBufferManagementInfo.pPoolHandles,0,sizeof(ClPoolT)*pBufferPoolConfigUser->numPools);

    memcpy(pBufferPoolConfig->pPoolConfig,
           pBufferPoolConfigUser->pPoolConfig,
           sizeof(*pBufferPoolConfig->pPoolConfig) * pBufferPoolConfigUser->numPools);

    /*
      keep the list sorted
      to accomodate a binary search
      while allocating a chunk.
    */

    qsort(pBufferPoolConfig->pPoolConfig,pBufferPoolConfig->numPools,
          sizeof(*pBufferPoolConfig->pPoolConfig),clBufferCmp);

    if( (rc = clBufferValidateConfig(pBufferPoolConfig)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Buffer config validation failed\n"));
        goto out_free;
    }

    clBufferModeSet();

    rc = CL_OK;
    goto out;

    out_free:
    if(pBufferPoolConfig->pPoolConfig)
    {
        clHeapFree(pBufferPoolConfig->pPoolConfig);
    }
    if(gBufferManagementInfo.pPoolHandles)
    {
        clHeapFree(gBufferManagementInfo.pPoolHandles);
    }
    gBufferManagementInfo.pPoolHandles = 0;
    pBufferPoolConfig->pPoolConfig = NULL;

    out:
    return rc;
}

static ClRcT 
clBufferPoolFinalize(void)
{
    ClBufferPoolConfigT *pBufferPoolConfig;
    ClRcT rc = CL_OK;
    pBufferPoolConfig = &gBufferManagementInfo.bufferPoolConfig;
    clBufferModeUnset();
    clHeapFree(pBufferPoolConfig->pPoolConfig);
    clHeapFree(gBufferManagementInfo.pPoolHandles);
    return rc;
}

static ClRcT clBMBufferPoolStatsGet(ClUint32T numPools,ClUint32T *pPoolSize,ClPoolStatsT *pPoolStats)
{
    memset(pPoolStats,0,sizeof(*pPoolStats) * numPools);
    *pPoolSize = 0;
    return CL_OK;
}


