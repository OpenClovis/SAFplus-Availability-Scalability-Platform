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
#include <string.h>
#include <clCommonErrors.h>
#include <clOsalErrors.h>
#include <clDebugApi.h>
#include <clLogUtilApi.h>
#include <clBitmapApi.h>
#include <clBitmapErrors.h>

static ClRcT
clBitmapRealloc(ClBitmapInfoT  *pBitmapInfo, 
                ClUint32T      nBits);

static ClRcT 
clBitmapInfoInitialize(ClBitmapInfoT  *pBitmapInfo, 
                       ClUint32T      bitmapLen);

static ClRcT
clBitmapRealloc(ClBitmapInfoT  *pBitmapInfo, 
                ClUint32T      bitNum)
{
    ClRcT      rc     = CL_OK;
    ClUint32T  nBytes = 0;
    ClUint32T  nInit  = 0;

    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Enter: %u", bitNum);

    nBytes = bitNum / CL_BM_BITS_IN_BYTE;
    ++nBytes; /* bitNum is 0-based */
    pBitmapInfo->pBitmap = (ClUint8T*) clHeapRealloc(pBitmapInfo->pBitmap, nBytes); 
    if( NULL == pBitmapInfo->pBitmap )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"clHeapRealloc() failed");
        return CL_BITMAP_RC(CL_ERR_NO_MEMORY); 
    }
    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Reallocated: %u from %u", nBytes, 
                   pBitmapInfo->nBytes);

    nInit = nBytes - pBitmapInfo->nBytes;
    memset(&(pBitmapInfo->pBitmap[pBitmapInfo->nBytes]), 0, nInit);
    pBitmapInfo->nBytes = nBytes;

    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Exit");
    return rc;
}

static ClRcT 
clBitmapInfoInitialize(ClBitmapInfoT  *pBitmapInfo, 
                       ClUint32T      bitNum)
{
    ClRcT          rc           = CL_OK;    

    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Enter");

    rc = clOsalMutexCreate(&(pBitmapInfo->bitmapLock));
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"clOsalMutexCreate() rc: %x", rc);
        return rc;
    }

    pBitmapInfo->pBitmap  = NULL;
    pBitmapInfo->nBytes   = 0;
    pBitmapInfo->nBits    = 0;
    pBitmapInfo->nBitsSet = 0;
    rc = clBitmapRealloc(pBitmapInfo, bitNum);    
    if( CL_OK != rc )
    {
        clOsalMutexDelete(pBitmapInfo->bitmapLock);
        return rc;
    }
    pBitmapInfo->nBits = bitNum + 1; /* bitNum is 0-based FIXME i felt this is
                                      wrong*/
    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Exit");
    return rc;
}

/*
 * clBitmapCreate()
 */
ClRcT
clBitmapCreate(ClBitmapHandleT  *phBitmap, 
               ClUint32T        bitNum)
{
    ClRcT          rc           = CL_OK;

    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Enter");

    if ( NULL == phBitmap)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Null Pointer");
        return CL_BITMAP_RC(CL_ERR_NULL_POINTER);
    }

    *phBitmap = (ClBitmapInfoT*) clHeapRealloc(NULL, sizeof(ClBitmapInfoT)); 
    if( NULL == *phBitmap)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"clHeapRealloc() failed");
        return CL_BITMAP_RC(CL_ERR_NO_MEMORY); 
    }
    
    rc = clBitmapInfoInitialize(*phBitmap, bitNum);
    if (rc != CL_OK)
    {
        clHeapFree(phBitmap);
        return rc;
    }
    
    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Exit");
    return rc; 
}

ClRcT
clBitmapDestroy(ClBitmapHandleT  hBitmap)
{
    ClRcT          rc           = CL_OK;
    ClBitmapInfoT  *pBitmapInfo = hBitmap;

    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Enter");

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Invalid Handle");
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    do
    {
        rc = clOsalMutexDelete(pBitmapInfo->bitmapLock);
    }while( CL_OSAL_ERR_MUTEX_EBUSY == rc );

    clHeapFree(pBitmapInfo->pBitmap);
    clHeapFree(pBitmapInfo);

    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Exit");
    return rc;
}

ClRcT 
clBitmapBitSet(ClBitmapHandleT  hBitmap,
               ClUint32T        bitNum)
{
    ClRcT          rc           = CL_OK;
    ClBitmapInfoT  *pBitmapInfo = hBitmap;
    ClUint32T      elementIdx   = 0;
    ClUint32T      bitIdx       = 0;

    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Enter");

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Invalid Handle");
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    rc = clOsalMutexLock(pBitmapInfo->bitmapLock);
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"clOsalMutexLock() rc: %x", rc); 
        return rc;
    }

    if( bitNum >= (pBitmapInfo->nBytes * CL_BM_BITS_IN_BYTE) )
    {
        rc = clBitmapRealloc(pBitmapInfo, bitNum);
        if( CL_OK != rc )
        { 
    	    clOsalMutexUnlock(pBitmapInfo->bitmapLock);
            return rc;
        }
    }
    if( bitNum >= pBitmapInfo->nBits )
    {
        pBitmapInfo->nBits = bitNum + 1;   
    }
    elementIdx = bitNum >> CL_BM_BITS_IN_BYTE_SHIFT;
    bitIdx = bitNum & CL_BM_BITS_IN_BYTE_MASK;
    (pBitmapInfo->pBitmap)[elementIdx] |= (0x1 << bitIdx);
    ++(pBitmapInfo->nBitsSet);

    clOsalMutexUnlock(pBitmapInfo->bitmapLock);   
    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Exit");
    return rc;
}

ClRcT 
clBitmapAllBitsSet(ClBitmapHandleT  hBitmap)
{
    ClRcT          rc           = CL_OK;
    ClBitmapInfoT  *pBitmapInfo = hBitmap;
    ClUint32T      elementIdx   = 0;
    ClUint32T      bitIdx       = 0;
    ClUint32T i;

    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Enter");

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Invalid Handle");
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    rc = clOsalMutexLock(pBitmapInfo->bitmapLock);
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"clOsalMutexLock() rc: %x", rc); 
        return rc;
    }

    for(i = 0; i < pBitmapInfo->nBits; ++i)
    {
        elementIdx = i >> CL_BM_BITS_IN_BYTE_SHIFT; 
        bitIdx = i & CL_BM_BITS_IN_BYTE_MASK;
        (pBitmapInfo->pBitmap)[elementIdx] |= (0x1 << bitIdx);
    }

    pBitmapInfo->nBitsSet = pBitmapInfo->nBits;

    clOsalMutexUnlock(pBitmapInfo->bitmapLock);   
    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Exit");
    return rc;
}

ClRcT 
clBitmapBitClear(ClBitmapHandleT  hBitmap,
                 ClUint32T        bitNum)
{
    ClRcT          rc           = CL_OK;
    ClBitmapInfoT  *pBitmapInfo = hBitmap;
    ClUint32T      elementIdx   = 0;
    ClUint32T      bitIdx       = 0;

    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Enter");

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Invalid Handle");
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    rc = clOsalMutexLock(pBitmapInfo->bitmapLock);
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"clOsalMutexLock() rc: %x", rc); 
        return rc;
    }

    if( bitNum >= pBitmapInfo->nBits )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Invalid Position");
        clOsalMutexUnlock(pBitmapInfo->bitmapLock);
        return CL_BITMAP_RC(CL_ERR_INVALID_PARAMETER);
    }

    elementIdx = bitNum >> CL_BM_BITS_IN_BYTE_SHIFT;
    bitIdx = bitNum & CL_BM_BITS_IN_BYTE_MASK;
    (pBitmapInfo->pBitmap)[elementIdx] &= ~(0x1 << bitIdx);
    --(pBitmapInfo->nBitsSet);
        
    clOsalMutexUnlock(pBitmapInfo->bitmapLock);   
    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Exit");
    return rc;
}

ClRcT 
clBitmapAllBitsClear(ClBitmapHandleT  hBitmap)
{
    ClRcT          rc           = CL_OK;
    ClBitmapInfoT  *pBitmapInfo = hBitmap;
    ClUint32T      elementIdx   = 0;
    ClUint32T      bitIdx       = 0;
    ClUint32T i;

    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Enter");

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Invalid Handle");
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    rc = clOsalMutexLock(pBitmapInfo->bitmapLock);
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"clOsalMutexLock() rc: %x", rc); 
        return rc;
    }

    for(i = 0; i < pBitmapInfo->nBits; ++i)
    {
        elementIdx = i >> CL_BM_BITS_IN_BYTE_SHIFT;
        bitIdx = i & CL_BM_BITS_IN_BYTE_MASK;
        (pBitmapInfo->pBitmap)[elementIdx] &= ~(0x1 << bitIdx);
    }

    pBitmapInfo->nBitsSet = 0;
        
    clOsalMutexUnlock(pBitmapInfo->bitmapLock);   
    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Exit");
    return rc;
}

/*
 * clBitmapIsBitSet()
 */
ClInt32T 
clBitmapIsBitSet(ClBitmapHandleT  hBitmap,
                 ClUint32T        bitNum,
                 ClRcT            *pRetCode) 
{
    ClBitmapInfoT  *pBitmapInfo = hBitmap;
    ClUint32T      elementIdx   = 0;
    ClUint32T      bitIdx       = 0;
    ClInt32T       bitVal       = CL_BM_BIT_UNDEF;
    
    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Enter");

    if( NULL == pRetCode )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"NULL value");
        return bitVal;
    }

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Invalid Handle");
        *pRetCode = CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
        return bitVal;
    }

    *pRetCode = clOsalMutexLock(pBitmapInfo->bitmapLock);
    if( CL_OK != *pRetCode )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED, 
                   "clOsalMutexLock() rc: %x", *pRetCode); 
        return bitVal;
    }

    if( bitNum >= pBitmapInfo->nBits )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Invalid Position");
        clOsalMutexUnlock(pBitmapInfo->bitmapLock);
        *pRetCode = CL_BITMAP_RC(CL_ERR_INVALID_PARAMETER);
        return bitVal;
    }

    elementIdx = bitNum / CL_BM_BITS_IN_BYTE;
    bitIdx = bitNum % CL_BM_BITS_IN_BYTE;
    bitVal = ((pBitmapInfo->pBitmap)[elementIdx] & (0x1 << bitIdx)) ? 
              CL_BM_BIT_SET : CL_BM_BIT_CLEAR;
    *pRetCode = CL_OK;

    clOsalMutexUnlock(pBitmapInfo->bitmapLock);
    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Exit");
    return bitVal;
}

ClRcT 
clBitmapWalk(ClBitmapHandleT  hBitmap, 
             ClBitmapWalkCbT  fpUserSetBitWalkCb, 
             void             *pCookie)
{    
    ClRcT          rc           = CL_OK;
    ClBitmapInfoT  *pBitmapInfo = hBitmap;
    ClUint32T      elementIdx   = 0;
    ClUint32T      bitIdx       = 0;
    ClUint32T      bitNum       = 0;
    
    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Enter");

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Invalid Handle");
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    if( NULL == fpUserSetBitWalkCb ) 
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"NULL parameter"); 
        return CL_BITMAP_RC(CL_ERR_NULL_POINTER);
    }

    rc = clOsalMutexLock(pBitmapInfo->bitmapLock);
    if( CL_OK != rc )
    {
        return rc;
    }

    for (bitNum = 0; bitNum < pBitmapInfo->nBits; ++bitNum)
    { 
        elementIdx = bitNum / CL_BM_BITS_IN_BYTE;
        bitIdx = bitNum % CL_BM_BITS_IN_BYTE;
        if( (pBitmapInfo->pBitmap)[elementIdx] & (0x1 << bitIdx) )
        {
            rc = fpUserSetBitWalkCb(hBitmap, bitNum, pCookie);
            if( CL_OK != rc )
            {
                break;
            }
        }
    }
        
    clOsalMutexUnlock(pBitmapInfo->bitmapLock);
    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Exit");
    return rc;
}

ClRcT 
clBitmapWalkUnlocked(ClBitmapHandleT  hBitmap, 
                     ClBitmapWalkCbT  fpUserSetBitWalkCb, 
                     void             *pCookie)
{    
    ClRcT          rc           = CL_OK;
    ClBitmapInfoT  *pBitmapInfo = hBitmap;
    ClUint32T      elementIdx   = 0;
    ClUint32T      bitIdx       = 0;
    ClUint32T      bitNum       = 0;
    ClUint8T *copyMap = NULL;
    ClUint32T nBits = 0;

    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Enter");

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Invalid Handle");
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    if( NULL == fpUserSetBitWalkCb ) 
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"NULL parameter"); 
        return CL_BITMAP_RC(CL_ERR_NULL_POINTER);
    }

    rc = clOsalMutexLock(pBitmapInfo->bitmapLock);
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"clOsalMutexLock() rc: %x", rc); 
        return rc;
    }
    nBits = pBitmapInfo->nBits;
    copyMap = (ClUint8T*) clHeapCalloc(pBitmapInfo->nBytes, sizeof(*copyMap));
    CL_ASSERT(copyMap != NULL);
    memcpy(copyMap, pBitmapInfo->pBitmap, pBitmapInfo->nBytes);
    clOsalMutexUnlock(pBitmapInfo->bitmapLock);

    for (bitNum = 0; bitNum < nBits; ++bitNum)
    { 
        elementIdx = bitNum / CL_BM_BITS_IN_BYTE;
        bitIdx = bitNum % CL_BM_BITS_IN_BYTE;
        if( copyMap[elementIdx] & (0x1 << bitIdx) )
        {
            rc = fpUserSetBitWalkCb(hBitmap, bitNum, pCookie);
            if( CL_OK != rc )
            {
                goto out_free;
            }
        }
    }
        
    out_free:
    if(copyMap)
        clHeapFree(copyMap);

    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Exit");
    return rc;
}

ClUint32T 
clBitmapLen(ClBitmapHandleT  hBitmap)
{
    ClBitmapInfoT *pBitmapInfo = hBitmap;

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        return 0;
    }

    return pBitmapInfo->nBits;
}

ClRcT
clBitmapNumBitsSet(ClBitmapHandleT  hBitmap,
                   ClUint32T        *pNumBits)
{
    ClBitmapInfoT *pBitmapInfo = hBitmap;

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Invalid Handle");
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    *pNumBits = pBitmapInfo->nBitsSet;
    return CL_OK;
}

ClRcT
clBitmapNextClearBitSetNGet(ClBitmapHandleT  hBitmap,
                            ClUint32T        length, 
                            ClUint32T        *pBitSet)
{
    ClRcT          rc           = CL_OK;
    ClBitmapInfoT  *pBitmapInfo = hBitmap;
    ClUint32T      byteIdx      = 0;
    ClUint32T      bitIdx       = 0;
    ClUint32T      bitNum       = 0;

    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Enter");

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Invalid Handle");
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    rc = clOsalMutexLock(pBitmapInfo->bitmapLock);
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"clOsalMutexLock() rc: %x", rc); 
        return rc;
    }

    if( length >= pBitmapInfo->nBits )
    {
        clOsalMutexUnlock(pBitmapInfo->bitmapLock);
        return CL_BITMAP_RC(CL_ERR_INVALID_PARAMETER);
    }

    for( bitNum = 0; bitNum < length; bitNum++)
    {
        byteIdx = bitNum / CL_BM_BITS_IN_BYTE;
        bitIdx  = bitNum % CL_BM_BITS_IN_BYTE;
        if( !( (*(pBitmapInfo->pBitmap + byteIdx)) & (0x1 << bitIdx)) )
        {
            (pBitmapInfo->pBitmap)[byteIdx] |= (0x1 << bitIdx);
            *pBitSet = bitNum;
            clOsalMutexUnlock(pBitmapInfo->bitmapLock);   
            return CL_OK;
        }
    }

    clOsalMutexUnlock(pBitmapInfo->bitmapLock);   
    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Exit");
    return CL_BITMAP_RC(CL_ERR_INVALID_PARAMETER);
}

ClRcT
clBitmap2BufferGet(ClBitmapHandleT  hBitmap,
                   ClUint32T        *pListLen,
                   ClUint8T         **ppPositionList)
{
    ClBitmapInfoT  *pBitmapInfo = hBitmap;
    ClRcT          rc           = CL_OK;

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Invalid Handle");
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    rc = clOsalMutexLock(pBitmapInfo->bitmapLock);
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"clOsalMutexLock() rc: %x", rc); 
        return rc;
    }
    *ppPositionList = (ClUint8T*) clHeapCalloc(pBitmapInfo->nBytes, sizeof(ClUint8T));
    if( NULL == *ppPositionList )
    {
        clOsalMutexUnlock(pBitmapInfo->bitmapLock);
        return rc;
    }
    memcpy(*ppPositionList, pBitmapInfo->pBitmap, pBitmapInfo->nBytes);
    *pListLen = pBitmapInfo->nBytes;

    clOsalMutexUnlock(pBitmapInfo->bitmapLock);
    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Exit");
    return rc;
}

ClRcT
clBitmapBuffer2BitmapGet(ClUint32T        listLen, 
                         ClUint8T         *pPositionList,
                         ClBitmapHandleT  *phBitmap)
{
    ClRcT      rc      = CL_OK;
    ClUint32T  nBits   = 0;
    ClUint32T  bitNum  = 0;
    ClUint32T  bitIdx  = 0;
    ClUint32T  byteIdx = 0;

    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Enter");

    if( (NULL == pPositionList) || (NULL == phBitmap) )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Null pointer");
        return CL_BITMAP_RC(CL_ERR_NULL_POINTER);
    }

    nBits = listLen * CL_BM_BITS_IN_BYTE;
    rc = clBitmapCreate(phBitmap, nBits);
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"clBitmapCreate(): rc[0x %x]", rc);
        return rc;
    }
    
    for(bitNum = 0; bitNum < nBits; bitNum++)
    {
       byteIdx = bitNum / CL_BM_BITS_IN_BYTE;
       bitIdx  = bitNum % CL_BM_BITS_IN_BYTE;
       if( (*(pPositionList + byteIdx)) & (0x1 << bitIdx) )
       {
           rc = clBitmapBitSet(*phBitmap, bitNum);
           if( CL_OK != rc )
           {
               ClRcT rc2 = CL_OK;
               clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"clBitmapBitSet(): rc[0x %x]",
                           rc);
               if( (rc2 = clBitmapDestroy(*phBitmap) ) != CL_OK )
               {
               }
               return rc;
           }
       }
    }

    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Exit");
    return rc;
}

ClRcT
clBitmap2PositionListGet(ClBitmapHandleT  hBitmap,
                         ClUint32T        *pListLen,
                         ClUint32T        **ppPositionList)
{
    ClBitmapInfoT  *pBitmapInfo = hBitmap;
    ClRcT          rc           = CL_OK;
    ClUint32T      count        = 0;
    ClUint32T      bitNum       = 0;
    ClUint32T      bitIdx       = 0;
    ClUint32T      byteIdx      = 0;

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Invalid Handle");
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    rc = clOsalMutexLock(pBitmapInfo->bitmapLock);
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"clOsalMutexLock() rc: %x", rc); 
        return rc;
    }
    *ppPositionList = (ClUint32T*) clHeapCalloc(pBitmapInfo->nBitsSet, sizeof(ClUint32T));
    if( NULL == *ppPositionList )
    {
        clOsalMutexUnlock(pBitmapInfo->bitmapLock);
        return rc;
    }
    for(bitNum = 0; bitNum < pBitmapInfo->nBits; bitNum++)
    {
       byteIdx = bitNum / CL_BM_BITS_IN_BYTE;
       bitIdx  = bitNum % CL_BM_BITS_IN_BYTE;
       if( (*(pBitmapInfo->pBitmap + byteIdx)) & (0x1 << bitIdx) )
       {
          (*ppPositionList)[count++] = bitNum;
       }
    }
    *pListLen = pBitmapInfo->nBitsSet;

    clOsalMutexUnlock(pBitmapInfo->bitmapLock);
    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Exit");
    return rc;
}

ClRcT
clBitmapPositionList2BitmapGet(ClUint32T        listLen, 
                               ClUint32T        *pPositionList,
                               ClBitmapHandleT  *phBitmap)
{
    ClRcT      rc      = CL_OK;
    ClUint32T  bitNum  = 0;
    ClUint32T  length  = 0;

    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Enter");

    if( (NULL == pPositionList) || (NULL == phBitmap) )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Null pointer");
        return CL_BITMAP_RC(CL_ERR_NULL_POINTER);
    }
    length = pPositionList[listLen - 1];
    rc = clBitmapCreate(phBitmap, length);
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"clBitmapCreate(): rc[0x %x]", rc);
        return rc;
    }
    
    for(bitNum = 0; bitNum < listLen; bitNum++)
    {
           rc = clBitmapBitSet(*phBitmap, pPositionList[bitNum]);
           if( CL_OK != rc )
           {
               ClRcT rc2 = CL_OK;
               clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"clBitmapBitSet(): rc[0x %x]",
                           rc);
               if( (rc2 = clBitmapDestroy(*phBitmap) ) != CL_OK)
               {
                   clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"clBitmapDestroy(): rc[%#x]",
                                                   rc2);
               }
               return rc;
           }
    }

    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Exit");
    return rc;
}

ClRcT
clBitmapBufferBitsCopy(ClUint32T        listLen, 
                       ClUint8T         *pPositionList,
                       ClBitmapHandleT  hBitmap)
{
    ClRcT      rc      = CL_OK;
    ClUint32T  nBits   = 0;
    ClUint32T  bitNum  = 0;
    ClUint32T  bitIdx  = 0;
    ClUint32T  byteIdx = 0;

    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Enter");

    if( NULL == pPositionList )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Null pointer");
        return CL_BITMAP_RC(CL_ERR_NULL_POINTER);
    }

    nBits = listLen * CL_BM_BITS_IN_BYTE;
    for(bitNum = 0; bitNum < nBits; bitNum++)
    {
       byteIdx = bitNum / CL_BM_BITS_IN_BYTE;
       bitIdx  = bitNum % CL_BM_BITS_IN_BYTE;
       if( (*(pPositionList + byteIdx)) & (0x1 << bitIdx) )
       {
           rc = clBitmapBitSet(hBitmap, bitNum);
           if( CL_OK != rc )
           {
               clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"clBitmapBitSet(): rc[0x %x]",
                           rc);
               return rc;
           }
       }
    }

    clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Exit");
    return rc;
}
