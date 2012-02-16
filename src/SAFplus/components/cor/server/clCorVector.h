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
 * ModuleName  : cor
 * File        : clCorVector.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains Vector DS API's & Definitions
 *
 *
 *****************************************************************************/

#ifndef _INC_COR_VECTOR_H_
#define _INC_COR_VECTOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
/** @pkg cl.cor.utils */

/* INCLUDES */

/* Macros for stream in and stream out data */


/* todo: need to worry about ntoh & hton later, right now
 * assumed to be the same endianness
 */
#define STREAM_OUT_STR(str, dst, len)   \
do\
{ \
  short l; \
  Byte_t* t; \
  STREAM_OUT(&l,(dst),sizeof(l)); \
  if(l>0) { \
    t = dst; \
    /*printf("Unpacking %d bytes at %s:%d\n", l, __FILE__, __LINE__);*/\
    dst += l; \
  } \
  if((str)) {\
    if(l>0 && len>0) { \
      memcpy((str),(t),(len<l?len:l)); \
    } \
    (str)[len<l?len-1:l]=0; /* add null */ \
  } \
} \
while(0)

#define STREAM_IN_STR(pBufH, str)     \
do \
{ \
  ClRcT rc = CL_OK; \
  short l = (str?strlen(str):0); \
  if((rc = clBufferNBytesWrite(*pBufH, (ClUint8T *)&l, sizeof(l))) != CL_OK) \
  { \
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while writing the data into buffer. rc[0x%x]", rc)); \
        return rc; \
  } \
  if(str) { \
    if((rc = clBufferNBytesWrite(*pBufH, (ClUint8T *)str, (l))) != CL_OK) \
    { \
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while writing the data into buffer. rc[0x%x]", rc)); \
        return rc; \
    } \
  } \
} \
while(0)

#define STREAM_IN_BOUNDS(dst, src, len, sizeLeft)    \
do \
{ \
  if(len>sizeLeft) { \
    CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_SET_RC(CL_COR_ERR_STR)(CL_COR_SET_RC(CL_COR_ERR_INSUFFICIENT_BUFFER)))); \
    return CL_COR_SET_RC(CL_COR_ERR_INSUFFICIENT_BUFFER); \
  } \
  STREAM_IN(dst, src, len); \
  sizeLeft -= len; \
} \
while(0)

#define STREAM_IN_BOUNDS_HTONS(pBufH, src, tmp16, len)    \
do \
{ \
  ClRcT rc = CL_OK; \
  tmp16 = htons(*src); \
  if((rc = clBufferNBytesWrite(*pBufH, (ClUint8T *)&tmp16, len) != CL_OK)) \
  { \
     CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while writing the data into buffer. rc[0x%x] ", rc)); \
     return rc; \
  } \
} \
while(0)

#define STREAM_IN_BOUNDS_HTONL(pBufH, src, tmp32, len)    \
do \
{ \
  ClRcT rc = CL_OK; \
  tmp32 = htonl(*src); \
  if((rc = clBufferNBytesWrite(*(pBufH), (ClUint8T *)&tmp32, len) != CL_OK)) \
  { \
     CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while writing the data into buffer. rc[0x%x]", rc)); \
     return rc; \
  } \
} \
while(0)

#define STREAM_IN_BUFFER(pBufH, src, len) \
do \
{ \
    ClRcT ret = CL_OK; \
    if((ret = clBufferNBytesWrite(*(pBufH), (ClUint8T *)src, len)) != CL_OK) \
    { \
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while packing the data in the buffer. rc[0x%x]", ret));\
        return ret;\
    }\
} while(0)
/*todo: convert these to TLV type, so it carries tag, type and later
 * can be interpreted as required.
 */
#define STREAM_IN(dst, src, len)    \
do {                                \
    /*printf("Packing %d bytes at %s:%d\n", len, __FILE__, __LINE__); */\
    memcpy(dst,src,len);            \
    (dst) = (Byte_t*)(dst) + (len);                   \
}while(0)
#define STREAM_IN_HTONS(dst, src, tmp16, len)    \
do {                                \
    /*printf("Packing %d bytes at %s:%d\n", len, __FILE__, __LINE__); */\
	tmp16 = htons(*src); \
    memcpy(dst,&tmp16,len);            \
    (dst) = (Byte_t*)(dst) + (len);                   \
}while(0)
#define STREAM_IN_BUFFER_HANDLE_HTONL(bufH, src, tmp32, len)    \
do {                                \
    /*printf("Packing %d bytes at %s:%d\n", len, __FILE__, __LINE__); */\
  ClRcT ret = CL_OK; \
	tmp32 = htonl(*src);\
    if(((ret = clBufferNBytesWrite((bufH), (ClUint8T *)&tmp32, len)) )!= CL_OK) \
    { \
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while packing the tag in the buffer. rc [0x%x]",ret));\
        return ret;\
    }\
}while(0)
#define STREAM_IN_BUFFER_HANDLE_HTONS(bufH, src, tmp16, len)    \
do {                                \
    /*printf("Packing %d bytes at %s:%d\n", len, __FILE__, __LINE__); */\
    ClRcT ret = CL_OK; \
	tmp16 = htons(*src); \
    if((ret = clBufferNBytesWrite((bufH), (ClUint8T *)&tmp16, len)) != CL_OK) \
    { \
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while packing the tag in the buffer. rc[0x%x]", ret));\
        return ret;\
    }\
}while(0)
#define STREAM_IN_HTONL(dst, src, tmp32, len)    \
do {                                \
    /*printf("Packing %d bytes at %s:%d\n", len, __FILE__, __LINE__); */\
	tmp32 = htonl(*src);\
    memcpy(dst,&tmp32,len);            \
    (dst) = (Byte_t*)(dst) + (len);                   \
}while(0)
#define STREAM_OUT(dst, src, len)   \
do {                                \
    /*printf("Unpacking %d bytes at %s:%d\n", len, __FILE__, __LINE__); */\
    memcpy(dst,src,len);            \
    (src) = (Byte_t *)(src) + (len);                   \
}while(0)

#define STREAM_OUT_NTOHS(dst, src, len)   \
do {                                \
    /*printf("Unpacking %d bytes at %s:%d\n", len, __FILE__, __LINE__); */\
    memcpy(dst,src,len);            \
	*dst = ntohs(*dst); \
    (src) = (Byte_t *)(src) + (len);                   \
}while(0)

#define STREAM_OUT_NTOHL(dst, src, len)   \
do {                                \
    /*printf("Unpacking %d bytes at %s:%d\n", len, __FILE__, __LINE__); */\
    memcpy(dst,src,len);            \
	*dst = ntohl(*dst);   \
    (src) = (Byte_t *)(src) + (len);                   \
}while(0)
#define STREAM_OUT_BOUNDS(dst, src, len, sizeLeft) \
do \
{ \
  STREAM_OUT(dst, src, len); \
  sizeLeft -= len; \
} \
while(0)

#define STREAM_OUT_BOUNDS_NTOHL(dst, src, len, sizeLeft) \
do \
{ \
  STREAM_OUT(dst, src, len); \
  *dst = ntohl(*dst); \
  sizeLeft -= len; \
} \
while(0)

#define STREAM_OUT_BOUNDS_NTOHS(dst, src, len, sizeLeft) \
do \
{ \
  STREAM_OUT(dst, src, len); \
  *dst = ntohs(*dst); \
  sizeLeft -= len; \
} \
while(0)

#define STREAM_PEEK(dst, src, len)      \
do {                                    \
    memcpy(dst,src,len);                    \
} while(0)

#define STREAM_PEEK_NTOHL(dst, src, len)      \
do {                                    \
    memcpy(dst,src,len);                    \
	*dst = ntohl(*dst); \
} while(0)

#define STREAM_PEEK_NTOHS(dst, src, len)      \
do {                                    \
    memcpy(dst,src,len);                    \
	*dst = ntohs(*dst);  \
} while(0)

typedef ClUint8T       Byte_t;
typedef Byte_t*        Byte_h;


/**
 *  Vector Data Type.
 *
 *  COR Vector Type.  Vector data type (aka array) implements a
 *  growable array of objects.  The vector elements can be accessed
 *  using an integer index.  The size of the vector can grow as needed
 *  to accomodate new elements. The current implementation of vector
 *  maintains blocks of elements, where the block size is determined
 *  by the application (and the blocks are linked).
 *
 *  The data holds 'blockSize' pointers.  Actual data is filled in by
 *  the user of vector.
 *
 *  @todo   Need to add shrink/compact routines, if necessary
 *
 */

struct CORVectorNode
{
  struct CORVectorNode* next;
  void *               data;             /*  Holds 'blockSize' items
                                             of size 'elementSize' */
};

typedef struct CORVectorNode CORVectorNode_t;
typedef CORVectorNode_t*     CORVectorNode_h;
typedef ClRcT (*CORVectorWalkFP)(ClUint32T idx, void * element, void ** cookie);
typedef ClRcT (*CORWalkUpFP)(ClUint32T idx, void * element, void ** cookie);

#define COR_FIRST_NODE_IDX   (0)
#define COR_LAST_NODE_IDX    (-1)
#define COR_FIRST_NODE_TAG   (0xEA2917EA)
#define COR_LAST_NODE_TAG    (0xEA7477EA)
#define COR_VECTOR_TAG       (0xEA636556)
#define COR_FIRST_DATA_IDX   (0)
#define COR_LAST_DATA_IDX    (-1)
#define COR_FIRST_DATA_TAG   (0xEA3782BC)
#define COR_LAST_DATA_TAG    (0xEA9613BC)
#define COR_FIRST_GROUP_IDX  (0)
#define COR_LAST_GROUP_IDX   (-1)
#define COR_FIRST_GROUP_TAG  (0xEA5298DA)
#define COR_LAST_GROUP_TAG   (0xEA7413DA)

typedef enum
  {
    COR_NORMAL_WALK = 0,
    COR_LAST_NODE_INDICATION_WALK = 1
  }
CORWalkType_e;

struct CORVector 
{
  ClUint16T        elementsPerBlock;         /**< Block Size of the vector  */
  ClUint32T        blocks;                   /**< Blocks of data 
                                                 total size is
                                                 (blocks*blockSize)  */
  ClUint32T        elementSize;              /**< Size of one element */
  CORVectorNode_h   head;                     /**< pointer to head  */
  CORVectorNode_h   tail;                     /**< pointer to tail  */
  ClCntHandleT      rbTree;                  /* red black tree to optimize the search */
#ifdef CL_COR_REDUCE_OBJ_MEM 
  CORVectorWalkFP   fpPack;                  /**< User Pack Function Pointer */
  CORVectorWalkFP   fpUnpack;                 /**< User Unpack Function Pointer */
  CORVectorWalkFP   fpDelete;                 /**< User delete Function Ptr */
#endif 
};
  
typedef struct CORVector  CORVector_t;
typedef CORVector_t*      CORVector_h;


/**
 * Get Size of Vector.
 * Returns the size of the vector (max elements it can accomodate)
 */
#define corVectorSizeGet(v)    ((v).elementsPerBlock*(v).blocks)

/**
 * Get Size of Element.
 * Returns the size of an element in the vector.
 */
#define corVectorElementSizeGet(v)    ((v).elementSize)

#ifdef CL_COR_REDUCE_OBJ_MEM
/**
 * Set the pack function.
 * Sets the pack function for the vector.
 */
#define corVectorPackFPSet(v,fp) ((v).fpPack=fp)
/**
 * Set the unpack function.
 * Sets the Unpack function for the vector.
 */
#define corVectorUnpackFPSet(v,fp) ((v).fpUnpack=fp)
/**
 * Set the delete function.
 * Sets the delte function for the vector.
 */
#define corVectorDeleteFPSet(v,fp) ((v).fpDelete=fp)

#endif
/**
 * cor vector walk function.
 * Walk functionality for cor vector.
 */
#define corVectorWalk(v,fp,c) (_corVectorWalk(v,fp,c,COR_NORMAL_WALK))



/**@#-*/

/* Function Prototypes */

/* constructor/destructor functions */

#ifdef COR_TEST
ClRcT                corVectorCreate(ClUint32T elementSize,
                                      ClUint16T blockSize,
                                      CORVector_h* instance);
#endif
ClRcT                corVectorInit(CORVector_h this,
                                    ClUint32T elementSize,
                                    ClUint16T blockSize);
ClRcT                corVectorRemoveAll(CORVector_h this);
#if 0
ClRcT                corVectorDelete(CORVector_h this);
#endif

/* public functions */

void *                corVectorGet(CORVector_h this,
                                   ClUint32T  idx);
void *                corVectorExtendedGet(CORVector_h this,
                                           ClUint32T  idx);
ClRcT                corVectorPack(CORVector_h this,
                                    void *  dst);
ClRcT                corVectorUnpack(CORVector_h this,
                                      void *  buf,
                                      ClUint32T* size);
ClRcT                _corVectorWalk(CORVector_h this,
                                     CORVectorWalkFP fp,
                                     void ** cookie,
                                     CORWalkType_e type
                                     );
ClRcT 				  _corWalkUp(CORVector_h this,
               					 CORWalkUpFP fp,
               					 void ** cookie,
								 CORWalkType_e type);

ClRcT                corVectorExtend(CORVector_h this,
                                      ClUint32T  size);

ClRcT                corVectorNodeFind(CORVector_h this, ClUint32T id, ClUint32T index, ClUint32T *idx);
#ifdef COR_TEST
void                  corVectorShow(CORVector_h this);
#endif

/**@#+*/
  

#ifdef __cplusplus
}
#endif

#endif  /*  _INC_COR_VECTOR_H_ */
