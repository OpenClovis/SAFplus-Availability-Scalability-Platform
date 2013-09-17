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

#include <clDifferenceVector.h>
#include <clCksmApi.h>
#include <clLogApi.h>
#include <clDebugApi.h>

#define CL_MD5_BLOCK_SHIFT (CL_DIFFERENCE_VECTOR_BLOCK_SHIFT)
#define CL_MD5_BLOCK_SIZE (CL_DIFFERENCE_VECTOR_BLOCK_SIZE)
#define CL_MD5_BLOCK_MASK (CL_DIFFERENCE_VECTOR_BLOCK_MASK)

#define CL_DIFFERENCE_VECTOR_TABLE_BITS (10)
#define CL_DIFFERENCE_VECTOR_TABLE_SIZE ( 1 << CL_DIFFERENCE_VECTOR_TABLE_BITS )
#define CL_DIFFERENCE_VECTOR_TABLE_MASK (CL_DIFFERENCE_VECTOR_TABLE_SIZE-1)

#define __VECTOR_KEY_VALIDATE(key)                                      \
    (!(key) || !(key)->groupKey || !(key)->groupKey->length || !(key)->sectionKey || !(key)->sectionKey->pValue || !(key)->sectionKey->length)

static struct hashStruct *differenceVectorTable[CL_DIFFERENCE_VECTOR_TABLE_SIZE];

static ClDifferenceBlockT *differenceVectorFind(ClDifferenceVectorKeyT *key, ClUint32T *pHashKey)
{
    struct hashStruct *iter;
    ClUint32T hashKey;
    ClUint32T cksum1 = 0;
    ClUint32T cksum2 = 0;
    clCksm32bitCompute((ClUint8T*)key->groupKey->pValue, key->groupKey->length, &cksum1);
    clCksm32bitCompute((ClUint8T*)key->sectionKey->pValue, key->sectionKey->length, &cksum2);
    hashKey = (cksum1 ^ cksum2 ) & CL_DIFFERENCE_VECTOR_TABLE_MASK;
    if(pHashKey)
        *pHashKey = hashKey;
    for(iter = differenceVectorTable[hashKey]; iter; iter = iter->pNext)
    {
        ClDifferenceBlockT *block = hashEntry(iter, ClDifferenceBlockT, hash);
        if(block->key.groupKey->length != key->groupKey->length) continue;
        if(block->key.sectionKey->length != key->sectionKey->length) continue;
        if(memcmp(block->key.groupKey->pValue, key->groupKey->pValue, key->groupKey->length)) continue;
        if(memcmp(block->key.sectionKey->pValue, key->sectionKey->pValue, key->sectionKey->length)) continue;
        return block;
    }
    return NULL;
}

static ClDifferenceBlockT *differenceVectorAdd(ClDifferenceVectorKeyT *key,
                                               ClBoolT *lastStatus)
{
    ClDifferenceBlockT *block;
    ClUint32T hashKey = 0;
    ClBoolT status = CL_TRUE;

    block = differenceVectorFind(key, &hashKey);
    if(!block)
    {
        block = clHeapCalloc(1, sizeof(*block));
        CL_ASSERT(block != NULL);
        block->key.groupKey = clStringDup(key->groupKey);
        CL_ASSERT(block->key.groupKey != NULL);
        block->key.sectionKey = clStringDup(key->sectionKey);
        CL_ASSERT(block->key.sectionKey != NULL);
        hashAdd(differenceVectorTable, hashKey, &block->hash);
        status = CL_FALSE;
    }

    if(lastStatus)
        *lastStatus = status;

    return block;
}

static ClUint32T __differenceVectorGet(ClDifferenceBlockT *block, ClUint8T *data, ClOffsetT offset, 
                                       ClSizeT dataSize, ClDifferenceVectorT *differenceVector)
{
    ClInt32T i;
    ClMD5T *md5CurList = block->md5List;
    ClMD5T *md5List ;
    ClUint32T md5CurBlocks = block->md5Blocks;
    ClUint32T md5Blocks = 0;
    ClUint32T dataBlocks, sectionBlocks ;
    ClSizeT sectionSize;
    ClSizeT vectorSize = 0;
    ClSizeT lastDataSize = dataSize;
    ClUint8T *pLastData = NULL;
    ClInt32T lastMatch = -1;
    ClBoolT doMD5 = CL_FALSE;
    ClUint32T startBlock, endBlock;
    ClOffsetT startOffset, nonZeroOffset = 0;

    sectionSize = offset + dataSize;
    /*
     * First align to md5 block size
     */
    dataBlocks = ((dataSize + CL_MD5_BLOCK_MASK) & ~CL_MD5_BLOCK_MASK) >> CL_MD5_BLOCK_SHIFT;
    sectionBlocks = ((sectionSize + CL_MD5_BLOCK_MASK) & ~CL_MD5_BLOCK_MASK) >> CL_MD5_BLOCK_SHIFT;
    startOffset = offset & CL_MD5_BLOCK_MASK;
    startBlock = ( offset & ~CL_MD5_BLOCK_MASK ) >> CL_MD5_BLOCK_SHIFT; /* align the start block*/
    endBlock = sectionBlocks;

    md5Blocks = CL_MAX(sectionBlocks, md5CurBlocks);
    md5List = clHeapCalloc(md5Blocks, sizeof(*md5List));
    CL_ASSERT(md5List != NULL);
    
    if(md5CurList)
        memcpy(md5List, md5CurList, md5CurBlocks);
    else 
    {
        md5CurList = md5List;
        md5CurBlocks = md5Blocks;
    }

    /*
     * If the specified vector blocks don't match the data blocks.
     * refill. Reset the md5 for offsetted writes considering its cheaper to 
     * just recompute the md5 for this block on a subsequent write to this block
     */
    if(differenceVector && differenceVector->md5Blocks && differenceVector->md5Blocks == dataBlocks)
    {
        memcpy(md5List + startBlock, differenceVector->md5List, sizeof(*md5List) * differenceVector->md5Blocks);
        /*
         *If data vector already specified, then just update the md5 list and exit.
         */
        if(differenceVector->numDataVectors) 
        {
            clLogTrace("DIFF", "MD5", "Difference vector already specified with md5 list of size [%d] "
                       "with [%d] difference data vectors", dataBlocks, differenceVector->numDataVectors);
            goto out_set;
        }
    }
    else doMD5 = CL_TRUE;

    data += offset;
    pLastData = data;
    
    /*
     * If we are going to allocate datavectors, free the existing set to be overridden
     * with a fresh set.
     */
    if(differenceVector && differenceVector->dataVectors)
    {
        clHeapFree(differenceVector->dataVectors);
        differenceVector->dataVectors = NULL;
        differenceVector->numDataVectors = 0;
    }

    nonZeroOffset |= startOffset;

    for(i = startBlock; i < endBlock; ++i)
    {
        ClSizeT c = CL_MIN(CL_MD5_BLOCK_SIZE - startOffset, dataSize);

        nonZeroOffset &= startOffset;

        if(doMD5)
        { 
            if(!startOffset)
                clMD5Compute(data, c, md5List[i].md5sum);
            else memset(&md5List[i], 0, sizeof(md5List[i]));
        }

        dataSize -= c;
        data += c;

        if(startOffset) 
            startOffset = 0;

        /*
         * Just gather the new md5 list if there is no vector to be accumulated
         */
        if(!differenceVector) continue;

        /*
         * Just gather md5s if we hit the limit for the current data size or if
         * we didnt have an md5 to start with
         */
        if(md5List == md5CurList)
        {
            if(lastMatch < 0) lastMatch = i;
            continue;
        }

        if(i < md5CurBlocks)
        {
            /*
             * Always store offsetted blocks in the difference vector. whose md5 wasnt computed.
             */
            if(!nonZeroOffset && memcmp(md5List[i].md5sum, md5CurList[i].md5sum, sizeof(md5List[i].md5sum)) == 0)
            {
                /*
                 * Blocks are the same. Skip the add for this block.
                 */
                clLogTrace("DIFF", "MD5", "Skipping copying block [%d] to replica", i);
                continue;
            }
        }
        else
        {
            if(lastMatch < 0)
            {
                lastMatch = i;
                pLastData = data - c;
                lastDataSize = dataSize + c;
            }
            continue;
        }

        clLogTrace("DIFF", "MD5", "Copying block [%d] to replica of size [%lld]", i, c);

        if(!(differenceVector->numDataVectors & 7 ) )
        {
            differenceVector->dataVectors = clHeapRealloc(differenceVector->dataVectors,
                                                          sizeof(*differenceVector->dataVectors) * 
                                                          (differenceVector->numDataVectors + 8));
            CL_ASSERT(differenceVector->dataVectors != NULL);
            memset(differenceVector->dataVectors + differenceVector->numDataVectors, 0, 
                   sizeof(*differenceVector->dataVectors) * 8);
        }
        differenceVector->dataVectors[differenceVector->numDataVectors].dataBlock = i; /* block mismatched */
        differenceVector->dataVectors[differenceVector->numDataVectors].dataBase = data - c;
        differenceVector->dataVectors[differenceVector->numDataVectors].dataSize = c;
        ++differenceVector->numDataVectors;
        vectorSize += c;
    }

    CL_ASSERT(dataSize == 0);

    if(lastMatch >= 0 && differenceVector) /* impossible but coverity killer : Who knows! */
    {
        if(!(differenceVector->numDataVectors & 7))
        {
            differenceVector->dataVectors = clHeapRealloc(differenceVector->dataVectors,
                                                          sizeof(*differenceVector->dataVectors) * 
                                                          (differenceVector->numDataVectors + 8));
            CL_ASSERT(differenceVector->dataVectors != NULL);
            memset(differenceVector->dataVectors + differenceVector->numDataVectors, 0, 
                   sizeof(*differenceVector->dataVectors) * 8);
        }
        clLogTrace("DIFF", "MD5", "Copying block [%d] to replica of size [%lld]", lastMatch, lastDataSize);
        differenceVector->dataVectors[differenceVector->numDataVectors].dataBlock = lastMatch;
        differenceVector->dataVectors[differenceVector->numDataVectors].dataBase = pLastData;
        differenceVector->dataVectors[differenceVector->numDataVectors].dataSize = lastDataSize;
        ++differenceVector->numDataVectors;
        vectorSize += lastDataSize;
    }

    if(differenceVector)
    {
        clLogTrace("DIFF", "MD5", "Vector has [%lld] bytes to be written. Skipped [%lld] bytes.",
                   vectorSize, sectionSize - vectorSize);
    }

    out_set:
    block->md5List = md5List;
    block->md5Blocks = md5Blocks;

    if(doMD5 && differenceVector)
    {
        clLogTrace("DIFF", "MD5", "Copying md5 list preloaded with [%d] blocks to the difference vector "
                   "with [%d] data difference vectors", dataBlocks, differenceVector->numDataVectors);
        if(differenceVector->md5List)
            clHeapFree(differenceVector->md5List);
        differenceVector->md5List = clHeapCalloc(dataBlocks, sizeof(*differenceVector->md5List));
        CL_ASSERT(differenceVector->md5List != NULL);
        memcpy(differenceVector->md5List, md5List + startBlock, sizeof(*differenceVector->md5List) * dataBlocks);
        differenceVector->md5Blocks = dataBlocks;
    }

    if(md5CurList != md5List)
        clHeapFree(md5CurList);

    return sectionBlocks;
}

/*
 * Merge the difference vector with the current data set
 */
static ClUint8T *__differenceVectorMerge(ClUint8T *lastData, ClSizeT lastDataSize,
                                         ClDifferenceVectorT *vector,
                                         ClOffsetT offset, ClSizeT dataSize)
{
    ClUint8T *mergeSpace = NULL;
    ClUint32T i;
    ClUint32T sectionBlocks;
    ClOffsetT startOffset;
    ClSizeT sectionSize;

    sectionSize = offset + dataSize;
    sectionBlocks = ( (sectionSize + CL_MD5_BLOCK_MASK) & ~CL_MD5_BLOCK_MASK ) >> CL_MD5_BLOCK_SHIFT;
    startOffset = offset & CL_MD5_BLOCK_MASK;

    /*
     * Merge the data into the section. 
     * Cannot reallocate as the allocation span can be more than the size of the section considering we reuse
     * the merge space if the current section size is already bigger than the specified section size.
     */
    if(sectionSize > lastDataSize)
    {
        mergeSpace = clHeapCalloc(1, sectionSize);
        CL_ASSERT(mergeSpace != NULL); 
        if(lastData)
            memcpy(mergeSpace, lastData, lastDataSize);
    }
    else mergeSpace = lastData;  /*reuse the old section allocation and overwrite the data*/

    /*
     * Now apply the difference.
     */
    for(i = 0; i < vector->numDataVectors; ++i)
    {
        ClUint32T block = vector->dataVectors[i].dataBlock;
        ClSizeT size = vector->dataVectors[i].dataSize;
        ClUint8T *pData = vector->dataVectors[i].dataBase;
        CL_ASSERT(block < sectionBlocks); /* validation of the specified block size*/
        CL_ASSERT((block << CL_MD5_BLOCK_SHIFT) + size <= sectionSize);
        clLogTrace("DIFF", "MD5-MERGE", "Copy new block [%d], size [%lld]", block, size);
        memcpy(mergeSpace + (block << CL_MD5_BLOCK_SHIFT) + startOffset, pData, size);
        if(startOffset) startOffset = 0; /*reset startOffset*/
        dataSize -= size;
    }
    CL_ASSERT((ClInt64T)dataSize >= 0);
    clLogTrace("DIFF", "MD5-MERGE", "Merged [%lld] bytes from old block at offset [%lld]", dataSize, offset);
    return mergeSpace;
}

static ClUint8T *differenceVectorMerge(ClDifferenceVectorKeyT *key, ClDifferenceVectorT *vector, ClOffsetT offset,
                                       ClSizeT size, ClDifferenceBlockT **pBlock)
{
    ClDifferenceBlockT *block;
    ClUint8T *mergeSpace;
    block = differenceVectorAdd(key, NULL);
    CL_ASSERT(block != NULL);
    mergeSpace = __differenceVectorMerge(block->data, block->size, vector, offset, size);
    if(block->data != mergeSpace)
    {
        clHeapFree(block->data);
        block->data = mergeSpace;
    }
    if(block->size < size)
        block->size = size;
    if(pBlock) *pBlock = block;
    return mergeSpace;
}

ClUint8T *clDifferenceVectorMergeWithData(ClUint8T *lastData, ClSizeT lastDataSize,
                                          ClDifferenceVectorT *vector, ClOffsetT offset, ClSizeT size)
{
    if(!vector) return lastData;
    return __differenceVectorMerge(lastData, lastDataSize, vector, offset, size);
}

ClUint8T *clDifferenceVectorMerge(ClDifferenceVectorKeyT *key, ClDifferenceVectorT *vector,
                                  ClOffsetT offset, ClSizeT size)
{
    if(__VECTOR_KEY_VALIDATE(key)) return NULL;
    return differenceVectorMerge(key, vector, offset, size, NULL);
}

ClUint8T *clDifferenceVectorMergeWithReset(ClDifferenceVectorKeyT *key, ClDifferenceVectorT *vector,
                                           ClOffsetT offset, ClSizeT size)
{
    ClDifferenceBlockT *block = NULL;
    ClUint8T *mergeSpace;
    if(__VECTOR_KEY_VALIDATE(key)) return NULL;
    mergeSpace = differenceVectorMerge(key, vector, offset, size, &block);
    if(block)
        block->size = size;
    return mergeSpace;
}

static ClUint32T differenceVectorGet(ClDifferenceVectorKeyT *key, ClUint8T *data, ClOffsetT offset, ClSizeT size,
                                     ClDifferenceVectorT *vector, ClBoolT copyData,
                                     ClDifferenceBlockT **pBlock)
{
    ClDifferenceBlockT *block;
    block = differenceVectorAdd(key, NULL);
    CL_ASSERT(block != NULL);
    if(pBlock) *pBlock = block;
    /*
     * Store the data too if enabled. Heavy though coz of duplication.
     */
    if(copyData)
    {
        if(block->size < offset+size)
        {
            if(block->data) clHeapFree(block->data);
            block->data = clHeapCalloc(1, offset+size);
            CL_ASSERT(block->data != NULL);
            block->size = offset+size;
        }
        if(block->data)
            memcpy(block->data + offset, data + offset, size);
    }
    return __differenceVectorGet(block, data, offset, size, vector);
}

ClRcT clDifferenceVectorGet(ClDifferenceVectorKeyT *key, ClUint8T *data, ClOffsetT offset, ClSizeT size, 
                            ClBoolT copyData, ClDifferenceVectorT *vector)
{
    if(__VECTOR_KEY_VALIDATE(key))
        return CL_ERR_INVALID_PARAMETER;
    if(!data || !size)
        return CL_ERR_INVALID_PARAMETER;
    (void)differenceVectorGet(key, data, offset, size, vector, copyData, NULL);
    return CL_OK;
}

ClRcT clDifferenceVectorGetWithReset(ClDifferenceVectorKeyT *key, ClUint8T *data, ClOffsetT offset, 
                                     ClSizeT size, ClBoolT copyData, ClDifferenceVectorT *vector)
{
    ClDifferenceBlockT *block = NULL;
    ClUint32T md5Blocks = 0;
    if(__VECTOR_KEY_VALIDATE(key))
        return CL_ERR_INVALID_PARAMETER;
    if(!data || !size)
        return CL_ERR_INVALID_PARAMETER;
    md5Blocks = differenceVectorGet(key, data, offset, size, vector, copyData, &block);
    CL_ASSERT(block != NULL);

    if(copyData || block->data)
        block->size = offset + size; /* reset the block size */

    block->md5Blocks = md5Blocks; /*reset it to the current specified data block range if overwriting*/
    return CL_OK;
}

void clDifferenceVectorFree(ClDifferenceVectorT *differenceVector, ClBoolT freeDataVector)
{
    ClUint32T i;
    if(!differenceVector) return;
    if(freeDataVector)
    {
        for(i = 0; i < differenceVector->numDataVectors; ++i)
        {
            if(differenceVector->dataVectors[i].dataBase)
                clHeapFree(differenceVector->dataVectors[i].dataBase);
        }
    }
    if(differenceVector->dataVectors)
        clHeapFree(differenceVector->dataVectors);
    if(differenceVector->md5List)
        clHeapFree(differenceVector->md5List);
}

void clDifferenceVectorCopy(ClDifferenceVectorT *dest, ClDifferenceVectorT *src)
{
    if(!dest || !src) return;
    memcpy(dest, src, sizeof(*dest));
    if(src->dataVectors)
    {
        if(src->numDataVectors)
        {
            dest->dataVectors = clHeapCalloc(src->numDataVectors,
                                             sizeof(*dest->dataVectors));
            CL_ASSERT(dest->dataVectors != NULL);
            memcpy(dest->dataVectors, src->dataVectors,
                   sizeof(*dest->dataVectors) * src->numDataVectors);
        }
        else dest->dataVectors = NULL;
    }
    else dest->numDataVectors = 0;
    if(src->md5List)
    {
        if(src->md5Blocks)
        {
            dest->md5List = clHeapCalloc(src->md5Blocks, sizeof(*dest->md5List));
            CL_ASSERT(dest->md5List != NULL);
            memcpy(dest->md5List, src->md5List, sizeof(*dest->md5List) * src->md5Blocks);
        }
        else dest->md5List = NULL;
    }
    else dest->md5Blocks = 0;

}

ClDifferenceVectorKeyT *clDifferenceVectorKeyMake(ClDifferenceVectorKeyT *key,
                                                  const SaNameT *groupKey, 
                                                  const ClCharT *sectionFmt, ...)
{
    ClInt32T sectionLen = 0;
    ClCharT *sectionKey = NULL;
    ClCharT c;
    va_list arg;
    
    if(!key)
    {
        key = clHeapCalloc(1, sizeof(*key));
        CL_ASSERT(key != NULL);
    }

    va_start(arg, sectionFmt);
    sectionLen = vsnprintf((ClCharT*)&c, 1, sectionFmt, arg);
    va_end(arg);

    if(!sectionLen) return NULL;
    
    ++sectionLen;
    sectionKey = clHeapCalloc(sectionLen, sizeof(*sectionKey));
    CL_ASSERT(sectionKey != NULL);
    
    va_start(arg, sectionFmt);
    vsnprintf(sectionKey, sectionLen, sectionFmt, arg);
    va_end(arg);

    key->groupKey = clHeapCalloc(1, sizeof(*key->groupKey));
    key->sectionKey = clHeapCalloc(1, sizeof(*key->sectionKey));
    
    CL_ASSERT(key->groupKey != NULL && key->sectionKey != NULL);
    key->groupKey->pValue = clHeapCalloc(1, groupKey->length);
    CL_ASSERT(key->groupKey->pValue != NULL);
    key->groupKey->length = groupKey->length;
    memcpy(key->groupKey->pValue, groupKey->value, key->groupKey->length);

    key->sectionKey->pValue = clHeapCalloc(1, strlen(sectionKey));
    CL_ASSERT(key->sectionKey->pValue != NULL);
    key->sectionKey->length = strlen(sectionKey);
    memcpy(key->sectionKey->pValue, sectionKey, key->sectionKey->length);

    clHeapFree(sectionKey);

    return key;
}

void clDifferenceVectorKeyFree(ClDifferenceVectorKeyT *key)
{
    if(!key) 
        return;
    if(key->groupKey)
    {
        if(key->groupKey->pValue)
            clHeapFree(key->groupKey->pValue);
        clHeapFree(key->groupKey);
    }
    if(key->sectionKey)
    {
        if(key->sectionKey->pValue)
            clHeapFree(key->sectionKey->pValue);
        clHeapFree(key->sectionKey);
    }
        
}

ClBoolT clDifferenceVectorKeyCheck(ClDifferenceVectorKeyT *key)
{
    return (ClBoolT)!!differenceVectorFind(key, NULL);
}

ClBoolT clDifferenceVectorKeyCheckAndAdd(ClDifferenceVectorKeyT *key)
{
    ClBoolT lastStatus = CL_FALSE;
    differenceVectorAdd(key, &lastStatus);
    return lastStatus;
}

static void differenceVectorDelete(ClDifferenceBlockT *block)
{
    hashDel(&block->hash);
    if(block->data) clHeapFree(block->data);
    clDifferenceVectorKeyFree(&block->key);
    if(block->md5List)
        clHeapFree(block->md5List);
    clHeapFree(block);
}

ClRcT clDifferenceVectorDelete(ClDifferenceVectorKeyT *key)
{
    ClDifferenceBlockT *block;
    if(__VECTOR_KEY_VALIDATE(key))
        return CL_ERR_INVALID_PARAMETER;
    block = differenceVectorFind(key, NULL);
    if(!block)
        return CL_ERR_NOT_EXIST;
    differenceVectorDelete(block);
    return CL_OK;
}

void clDifferenceVectorDestroy(void)
{
    ClUint32T i;
    for(i = 0; i < CL_DIFFERENCE_VECTOR_TABLE_SIZE; ++i)
    {
        struct hashStruct *iter;
        struct hashStruct *next = NULL;
        for(iter = differenceVectorTable[i]; iter; iter = next)
        {
            ClDifferenceBlockT *block = hashEntry(iter, ClDifferenceBlockT, hash);
            next = iter->pNext;
            differenceVectorDelete(block);
        }
        differenceVectorTable[i] = NULL;
    }
}
