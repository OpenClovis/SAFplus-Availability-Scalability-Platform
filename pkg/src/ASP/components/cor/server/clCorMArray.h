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
 * ModuleName  : cor
 * File        : clCorMArray.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains mArray DS API's & Definitions
 *
 *
 *****************************************************************************/

#ifndef _INC_M_ARRAY_H_
#define _INC_M_ARRAY_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @pkg cl.cor.utils */

/* INCLUDES */
#include <clCommon.h>
#include <clCorMetaData.h>
#include <clBufferApi.h>

/* Internal Headers*/
#include "clCorVector.h"

/**
 *  mArray Data Type.
 *
 *  mArray Data Structure. Each node of the mArray tree contains
 *  Vector of Vectors. Inturn, each Element in the leaf vector points
 *  to a node.
 *
 *  @todo   Need to add shrink/compact routines, if necessary
 *
 */

struct MArrayVector
{
  CORVector_t         nodes;
  ClUint16T          navigable; 
  ClUint32T			 numActiveNodes;
};


typedef struct MArrayVector  MArrayVector_t;
typedef MArrayVector_t*      MArrayVector_h;

#define MARRAY_GROUP_BLK_SZ  3
#define MARRAY_GROUP_NODE_SZ sizeof(MArrayVector_t)

#define MARRAY_NAVIGABLE_NODE 1
#define DEBUG_NAME_SZ       64

#define MOTREE	1
#define OBJTREE 2

struct MArrayNode
{
#ifdef DEBUG
  char                 name[DEBUG_NAME_SZ];
#endif

  ClUint32T           id;               /**< Non-zero Unique Identifier */
  CORVector_t          groups;           /**< each having vectors */
  void *               data;     
};

typedef struct MArrayNode MArrayNode_t;
typedef MArrayNode_t*     MArrayNode_h;
typedef CORVectorWalkFP   MArrayDataFP;
typedef MArrayDataFP      MArrayNodeFP;


struct MArray
{
#ifdef DEBUG
  char                 name[DEBUG_NAME_SZ];
#endif

  MArrayNode_h         root;             /**< root Node */
  ClUint32T           nodeSize;         /**< node data size */
  MArrayDataFP         fpPackData;       /**< User data Pack Function Pointer */
  MArrayNodeFP         fpPackNode;       /**< User node Pack Function Pointer */
  MArrayDataFP         fpUnpackData;     /**< User Unpack Data Function Pointer */
  MArrayNodeFP         fpUnpackNode;     /**< User Unpack Node Function Pointer */
#ifdef CL_COR_REDUCE_OBJ_MEM
  MArrayDataFP         fpDeleteData;     /**< User Delete Data Function Pointer */
  MArrayNodeFP         fpDeleteNode;     /**< User Delete Node Function Pointer */
#endif
};

typedef struct MArray  MArray_t;
typedef MArray_t*      MArray_h;


/**
 * Set the node pack function.
 * Sets the node pack function for the mArray.
 */
#define mArrayNodePackFPSet(ma,fp) ((ma).fpPackNode=(fp))
/**
 * Set the node unpack function.
 * Sets the node Unpack function for the mArray.
 */
#define mArrayNodeUnpackFPSet(ma,fp) ((ma).fpUnpackNode=(fp))

#ifdef CL_COR_REDUCE_OBJ_MEM
/**
 * Set the node delete function.
 * Sets the node delete function for the mArray.
 */
#define mArrayNodeDeleteFPSet(ma,fp) ((ma).fpDeleteNode=(fp))
#endif 


/**
 * Set the data pack function.
 * Sets the data pack function for the mArray.
 */
#define mArrayDataPackFPSet(ma,fp) ((ma).fpPackData=(fp))
/**
 * Set the data unpack function.
 * Sets the data Unpack function for the mArray.
 */
#define mArrayDataUnpackFPSet(ma,fp) ((ma).fpUnpackData=(fp))

#ifdef CL_COR_REDUCE_OBJ_MEM
/**
 * Set the data delete function.
 * Sets the data delete function for the mArray.
 */
#define mArrayDataDeleteFPSet(ma,fp) ((ma).fpDeleteData=(fp))
#endif



/**
 * mArray Walk.
 *
 * Walk thru the m-array.
 *
 * @param pMarray  Marray handle
 * @param fpNode   fp Node callback
 * @param fpData   fp Data callback
 * @param cookie   application context
 *
 */
#define mArrayWalk(idx, pMarray,nodePtr,dataPtr,ctxt, rc) \
do \
{ \
  MArrayWalk_t tmp; \
  tmp.pThis = (pMarray); \
  tmp.fpNode = (nodePtr); \
  tmp.fpData = (dataPtr); \
  tmp.cookie = (ctxt); \
  tmp.internal = 0; \
  tmp.flags = ((MArrayWalk_h)(*ctxt))->flags; \
   rc = _mArrayNodeWalk(idx, (pMarray)->root, (void **)&tmp); \
} \
while(0)

/**
 * mArray Delete.
 *
 * Walk thru the m-array and Delete the nodes.
 *
 * @param pMarray  Marray handle
 * @param fpNode   fp Node callback
 * @param fpData   fp Data callback
 * @param cookie   application context
 *
 */
#define mArrayDelete(idx, pMarray,nodePtr,dataPtr,ctxt, rc) \
do \
{ \
  MArrayWalk_t tmp; \
  tmp.pThis = (pMarray); \
  tmp.fpNode = (nodePtr); \
  tmp.fpData = (dataPtr); \
  tmp.cookie = (ctxt); \
  tmp.internal = 0; \
  tmp.flags = ((MArrayWalk_h)(*ctxt))->flags; \
   rc = _mArrayNodeDelete(idx, (pMarray)->root, (void **)&tmp); \
  clHeapFree((pMarray)->root); \
} \
while(0)

/**
 * mArray Group Get.
 * Get the group vector.
 */
#define mArrayGroupGet(this, groupId) \
  (MArrayVector_h)((this)?corVectorGet(&(this)->groups, (groupId)):0)



/* Internal macros */
#define COR_PRINT_SPACE(lvl) \
do\
{ \
  char space[10]; \
  char buff [160]; \
 \
  if((lvl)<150)\
    {\
      sprintf(space,"\n%%%dc",(lvl));\
      sprintf(buff,space,' ');\
    }\
}\
while(0)

#define COR_PRINT_SPACE_IN_BUFFER(lvl, buff) \
do\
{ \
  char space[10]; \
 \
  if((lvl)<150)\
    {\
      sprintf(space,"\n%%%dc",(lvl));\
      sprintf(buff,space,' ');\
    }\
}\
while(0)

/**
 * [Internal] mArray Internal Walk (with callback pointing to node
 * instead of node data.
 *
 */
#define _mArrayWalk(pMarray,nodePtr,dataPtr,ctxt) \
do \
{ \
  MArrayWalk_t tmp; \
  tmp.pThis = (pMarray); \
  tmp.fpNode = (nodePtr); \
  tmp.fpData = (dataPtr); \
  tmp.cookie = (ctxt); \
  tmp.internal = 1; \
  tmp.flags = ((MArrayWalk_h)(*ctxt))->flags; \
   _mArrayNodeWalk(0, (pMarray)->root, (void **)&tmp); \
} \
while(0)


/**@#-*/

/* temporary mArrayPack structure */
struct MArrayStream
{
  MArray_h     pThis;
  Byte_h       buf;
  ClBufferHandleT *pBufferHandle;
  ClUint32T   size;
  ClUint16T   flags;
  ClUint16T   type;
  CORVector_h  vec; 
};

typedef struct MArrayStream MArrayStream_t;
typedef MArrayStream_t* MArrayStream_h;

/* temporary walk structure */
struct MArrayWalk
{
  MArray_h          pThis;
  MArrayNodeFP      fpNode;
  MArrayDataFP      fpData;
  void **           cookie;
  ClUint32T        internal;
  ClRuleExprT* 	   expr;
  ClCorObjWalkFlagsT flags;
  ClBufferHandleT * pMsg;
};

typedef struct MArrayWalk MArrayWalk_t;
typedef MArrayWalk_t* MArrayWalk_h;


/* Function Prototypes */

/* constructor/destructor functions */

ClRcT                mArrayCreate(ClUint32T nodeSize,
                                   MArray_h* instance);
/* ClRcT                mArrayDelete(MArray_h this); */

ClRcT                mArrayNodeCreate(ClUint32T nodeSize,
                                       MArrayNode_h* instance);
ClRcT                mArrayNodeInit(MArrayNode_h this);
ClRcT                mArrayNodeDelete(MArrayNode_h this);
ClRcT _mArrayFreeNodeGet(ClUint32T idx,
                      void *     element, 
                      void **    cookie
                      );

#ifdef REL2
ClRcT                mArrayVectorCreate(ClUint32T elementSize,
                                         ClUint16T blockSize,
                                         ClUint16T navigable,
                                         MArrayVector_h* node);
ClRcT                mArrayVectorDelete(MArrayVector_h this);
#endif

/* public functions */

void *                mArrayDataGet(MArrayNode_h this,
                                    ClUint16T groupId,
                                    ClUint32T idx);
void *                mArrayExtendedDataGet(MArrayNode_h this,
                                            ClUint16T groupId,
                                            ClUint32T idx);
MArrayNode_h          mArrayNodeGet(MArrayNode_h this,
                                    ClUint16T groupId,
                                    ClUint32T idx);

ClRcT                mArrayNodeAdd(MArray_h tree,
                                    MArrayNode_h this,
                                    ClUint16T groupId,
                                    ClUint32T idx,
                                    ClUint32T blockSize);
ClRcT                mArrayDataNodeAdd(MArrayNode_h this,
                                        ClUint16T group,
                                        ClUint32T elemSize,
                                        ClUint32T blockSize);
ClRcT                mArrayDataNodeDelete(MArrayNode_h this,
                                        ClUint16T group);
ClRcT 		     _mArrayNodeDelete(ClUint32T nodeId,
                			void *     element,
                			void **    cookie
                			);

ClRcT                mArrayPack(MArray_h this,
				 ClUint16T flags,
				 ClUint16T type,
                        ClBufferHandleT *pBufH);
ClRcT                mArrayUnpack(MArray_h this,
				   ClUint32T type,
                                   void * src,
                                   ClUint32T* size);

ClRcT                _mArrayNodeWalk(ClUint32T  nodeIdx,
                                      void *  element,
                                      void ** cookie);
ClRcT                mArrayRemap(void * element);

ClRcT                mArrayUpdate(ClUint32T idx,
                                    void * element,
                                    void ** cookie);

void                  mArrayShow(MArray_h this, ClBufferHandleT *pMsgHdl);


/* utility functions */

MArrayNode_h          mArrayIdNodeFind(MArray_h this,
                                       ClUint32T* list,
                                       ClUint32T n);

MArrayNode_h          mArrayIdIdxNodeFind(MArray_h this,
                                          ClUint32T* list,
                                          ClUint32T* idx, 
                                          ClUint32T n);

ClRcT                mArrayId2Idx(MArray_h this, 
                                   ClUint32T* list, 
                                   ClUint32T n);
ClRcT                mArrayIdx2Id(MArray_h this, ClUint32T gid,
                                   ClUint32T* list, ClUint32T n);

ClRcT                mArrayNodeId2Idx(MArrayNode_h this, ClUint32T* nodeId);

void *     	      mArrayDataNodeFirstNodeGet(MArrayNode_h this,
						 ClUint32T   id);

MArrayNode_h          mArrayNodeIdNodeFind(MArrayNode_h this, ClUint32T id);
MArrayNode_h          mArrayNodeIdIdxNodeFind(MArrayNode_h this, 
                                              ClUint32T   id,
                                              ClUint32T   idx);
MArrayNode_h          mArrayNodeNextChildGet(MArrayNode_h this,
                                             ClUint32T *gid,
                                             ClInt32T*childIdx);
ClRcT               mArrayNodeIdGet(ClUint32T idx,
                                    void * element,
                                    void ** cookie);

#ifdef DEBUG
void                  mArrayNameSet(MArray_h this, char* name);
void                  mArrayNodeNameSet(MArrayNode_h this, char* name);
#endif

/**@#+*/
  


#ifdef __cplusplus
}
#endif

#endif  /*  _INC_M_ARRAY_H_ */
