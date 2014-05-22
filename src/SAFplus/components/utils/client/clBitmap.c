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

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Enter: %u", bitNum));

    nBytes = bitNum / CL_BM_BITS_IN_BYTE;
    ++nBytes; /* bitNum is 0-based */
    pBitmapInfo->pBitmap = clHeapRealloc(pBitmapInfo->pBitmap, nBytes); 
    if( NULL == pBitmapInfo->pBitmap )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clHeapRealloc() failed"));
        return CL_BITMAP_RC(CL_ERR_NO_MEMORY); 
    }
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Reallocated: %u from %u", nBytes, 
                   pBitmapInfo->nBytes));

    nInit = nBytes - pBitmapInfo->nBytes;
    memset(&(pBitmapInfo->pBitmap[pBitmapInfo->nBytes]), 0, nInit);
    pBitmapInfo->nBytes = nBytes;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Exit"));
    return rc;
}

static ClRcT 
clBitmapInfoInitialize(ClBitmapInfoT  *pBitmapInfo, 
                       ClUint32T      bitNum)
{
    ClRcT          rc           = CL_OK;    

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Enter"));

    rc = clOsalMutexCreate(&(pBitmapInfo->bitmapLock));
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clOsalMutexCreate() rc: %x", rc));
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
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Exit"));
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

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Enter"));

    if ( NULL == phBitmap)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null Pointer"));
        return CL_BITMAP_RC(CL_ERR_NULL_POINTER);
    }

    *phBitmap = clHeapRealloc(NULL, sizeof(ClBitmapInfoT)); 
    if( NULL == *phBitmap)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clHeapRealloc() failed"));
        return CL_BITMAP_RC(CL_ERR_NO_MEMORY); 
    }
    
    rc = clBitmapInfoInitialize(*phBitmap, bitNum);
    if (rc != CL_OK)
    {
        clHeapFree(phBitmap);
        return rc;
    }
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Exit"));
    return rc; 
}

ClRcT
clBitmapDestroy(ClBitmapHandleT  hBitmap)
{
    ClRcT          rc           = CL_OK;
    ClBitmapInfoT  *pBitmapInfo = hBitmap;
    
    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    do
    {
        rc = clOsalMutexDelete(pBitmapInfo->bitmapLock);
    }while( CL_OSAL_ERR_MUTEX_EBUSY == rc );

    clHeapFree(pBitmapInfo->pBitmap);
    clHeapFree(pBitmapInfo);

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

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Enter"));

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    rc = clOsalMutexLock(pBitmapInfo->bitmapLock);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clOsalMutexLock() rc: %x", rc)); 
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
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Exit"));
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

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Enter"));

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    rc = clOsalMutexLock(pBitmapInfo->bitmapLock);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clOsalMutexLock() rc: %x", rc)); 
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
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Exit"));
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

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Enter"));

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    rc = clOsalMutexLock(pBitmapInfo->bitmapLock);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clOsalMutexLock() rc: %x", rc)); 
        return rc;
    }

    if( bitNum >= pBitmapInfo->nBits )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Position"));
        clOsalMutexUnlock(pBitmapInfo->bitmapLock);
        return CL_BITMAP_RC(CL_ERR_INVALID_PARAMETER);
    }

    elementIdx = bitNum >> CL_BM_BITS_IN_BYTE_SHIFT;
    bitIdx = bitNum & CL_BM_BITS_IN_BYTE_MASK;
    (pBitmapInfo->pBitmap)[elementIdx] &= ~(0x1 << bitIdx);
    --(pBitmapInfo->nBitsSet);
        
    clOsalMutexUnlock(pBitmapInfo->bitmapLock);   
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Exit"));
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

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Enter"));

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    rc = clOsalMutexLock(pBitmapInfo->bitmapLock);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clOsalMutexLock() rc: %x", rc)); 
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
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Exit"));
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
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Enter"));

    if( NULL == pRetCode )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL value"));
        return bitVal;
    }

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
        *pRetCode = CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
        return bitVal;
    }

    *pRetCode = clOsalMutexLock(pBitmapInfo->bitmapLock);
    if( CL_OK != *pRetCode )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                       ("clOsalMutexLock() rc: %x", *pRetCode)); 
        return bitVal;
    }

    if( bitNum >= pBitmapInfo->nBits )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Position"));
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
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Exit"));
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
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Enter"));

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    if( NULL == fpUserSetBitWalkCb ) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL parameter")); 
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
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Exit"));
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

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Enter"));

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    if( NULL == fpUserSetBitWalkCb ) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL parameter")); 
        return CL_BITMAP_RC(CL_ERR_NULL_POINTER);
    }

    rc = clOsalMutexLock(pBitmapInfo->bitmapLock);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clOsalMutexLock() rc: %x", rc)); 
        return rc;
    }
    nBits = pBitmapInfo->nBits;
    copyMap = clHeapCalloc(pBitmapInfo->nBytes, sizeof(*copyMap));
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

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Exit"));
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
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
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

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Enter"));

    if( CL_BM_INVALID_BITMAP_HANDLE == hBitmap )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    rc = clOsalMutexLock(pBitmapInfo->bitmapLock);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clOsalMutexLock() rc: %x", rc)); 
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
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Exit"));
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
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    rc = clOsalMutexLock(pBitmapInfo->bitmapLock);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clOsalMutexLock() rc: %x", rc)); 
        return rc;
    }
    *ppPositionList = clHeapCalloc(pBitmapInfo->nBytes, sizeof(ClUint8T));
    if( NULL == *ppPositionList )
    {
        clOsalMutexUnlock(pBitmapInfo->bitmapLock);
        return rc;
    }
    memcpy(*ppPositionList, pBitmapInfo->pBitmap, pBitmapInfo->nBytes);
    *pListLen = pBitmapInfo->nBytes;

    clOsalMutexUnlock(pBitmapInfo->bitmapLock);
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Exit"));
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

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Enter"));

    if( (NULL == pPositionList) || (NULL == phBitmap) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null pointer"));
        return CL_BITMAP_RC(CL_ERR_NULL_POINTER);
    }

    nBits = listLen * CL_BM_BITS_IN_BYTE;
    rc = clBitmapCreate(phBitmap, nBits);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clBitmapCreate(): rc[0x %x]", rc));
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
               CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clBitmapBitSet(): rc[0x %x]",
                           rc));
               if( (rc2 = clBitmapDestroy(*phBitmap) ) != CL_OK )
               {
               }
               return rc;
           }
       }
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Exit"));
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
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
        return CL_BITMAP_RC(CL_ERR_INVALID_HANDLE);
    }

    rc = clOsalMutexLock(pBitmapInfo->bitmapLock);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clOsalMutexLock() rc: %x", rc)); 
        return rc;
    }
    *ppPositionList = clHeapCalloc(pBitmapInfo->nBitsSet, sizeof(ClUint32T));
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
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Exit"));
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

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Enter"));

    if( (NULL == pPositionList) || (NULL == phBitmap) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null pointer"));
        return CL_BITMAP_RC(CL_ERR_NULL_POINTER);
    }
    length = pPositionList[listLen - 1];
    rc = clBitmapCreate(phBitmap, length);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clBitmapCreate(): rc[0x %x]", rc));
        return rc;
    }
    
    for(bitNum = 0; bitNum < listLen; bitNum++)
    {
           rc = clBitmapBitSet(*phBitmap, pPositionList[bitNum]);
           if( CL_OK != rc )
           {
               ClRcT rc2 = CL_OK;
               CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clBitmapBitSet(): rc[0x %x]",
                           rc));
               if( (rc2 = clBitmapDestroy(*phBitmap) ) != CL_OK)
               {
                   CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clBitmapDestroy(): rc[%#x]",
                                                   rc2));
               }
               return rc;
           }
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Exit"));
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

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Enter"));

    if( NULL == pPositionList )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null pointer"));
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
               CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clBitmapBitSet(): rc[0x %x]",
                           rc));
               return rc;
           }
       }
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Exit"));
    return rc;
}
