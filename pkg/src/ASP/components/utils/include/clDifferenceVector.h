#ifndef _CL_DIFFERENCE_VECTOR_H_
#define _CL_DIFFERENCE_VECTOR_H_

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clMD5Api.h>
#include <clHash.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CL_DIFFERENCE_VECTOR_BLOCK_SHIFT (12)
#define CL_DIFFERENCE_VECTOR_BLOCK_SIZE ( 1 << CL_DIFFERENCE_VECTOR_BLOCK_SHIFT )
#define CL_DIFFERENCE_VECTOR_BLOCK_MASK ( CL_DIFFERENCE_VECTOR_BLOCK_SIZE - 1 )

typedef struct ClDifferenceVectorKey
{
    ClStringT *groupKey;
    ClStringT *sectionKey;
}ClDifferenceVectorKeyT;

typedef struct ClDifferenceBlock
{
    struct hashStruct hash; /*hash linkage*/
    ClDifferenceVectorKeyT key; /* key for the difference block*/
    ClSizeT size; /*optional last size*/
    ClUint8T *data;  /* optional last data*/
    ClUint32T md5Blocks;
    ClMD5T *md5List;
}ClDifferenceBlockT;

typedef struct ClDataVector
{
    ClUint32T dataBlock;
    ClSizeT dataSize;
    ClUint8T *dataBase;
}ClDataVectorT;

typedef struct ClDifferenceVector
{
    ClUint32T numDataVectors;
    ClDataVectorT *dataVectors;
    ClUint32T md5Blocks;
    ClMD5T *md5List;
}ClDifferenceVectorT;

extern 
ClRcT clDifferenceVectorGet(ClDifferenceVectorKeyT *key, ClUint8T *data, ClOffsetT offset, ClSizeT size, 
                            ClBoolT copyData, ClDifferenceVectorT *vector);

extern
ClRcT clDifferenceVectorGetWithReset(ClDifferenceVectorKeyT *key, ClUint8T *data, ClOffsetT offset, 
                                     ClSizeT size, ClBoolT copyData, ClDifferenceVectorT *vector);

extern
ClUint8T *clDifferenceVectorMergeWithData(ClUint8T *lastData, ClSizeT lastDataSize,
                                          ClDifferenceVectorT *vector, ClOffsetT offset, ClSizeT size);

extern
ClUint8T *clDifferenceVectorMerge(ClDifferenceVectorKeyT *key, ClDifferenceVectorT *vector,
                                  ClOffsetT offset, ClSizeT size);

extern
ClUint8T *clDifferenceVectorMergeWithReset(ClDifferenceVectorKeyT *key, ClDifferenceVectorT *vector,
                                           ClOffsetT offset, ClSizeT size);

extern
ClUint8T *clDifferenceVectorMergeWithData(ClUint8T *lastData, ClSizeT lastDataSize, ClDifferenceVectorT *vector,
                                          ClOffsetT offset, ClSizeT size);

extern
void clDifferenceVectorCopy(ClDifferenceVectorT *dest, ClDifferenceVectorT *src);

extern
ClRcT clDifferenceVectorDelete(ClDifferenceVectorKeyT *key);

extern
void clDifferenceVectorDestroy(void);

extern
void clDifferenceVectorKeyFree(ClDifferenceVectorKeyT *key);

extern
void clDifferenceVectorFree(ClDifferenceVectorT *differenceVector, ClBoolT freeDataVector);

#ifdef __cplusplus
}
#endif

#endif
