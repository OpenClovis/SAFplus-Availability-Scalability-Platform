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
 * File        : clCorTreeDefs.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This module contains pack, unpack APIs and definitions for object tree's data  elements.
 *
 *
 *****************************************************************************/

#ifndef _INC_TREE_DEF_H_
#define _INC_TREE_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @pkg cl.cor */

/* INCLUDES */
#include <clCommon.h>
#include <clCorServiceId.h>
#include <clBufferApi.h>

/* Internal Headers*/
#include "clCorMArray.h"
#include "clCorDmDefs.h"

/* DEFINES */

/**
 * [Internal] COR Managed Object Meta Structure.
 * Captures the Meta structure information for MO Class.
 *
 */
struct _CORMOClass {
  ClUint32T		numChildren;         /**< class identifier */
  ClUint16T        flags;               /**< flags yet to be defined */
  ClUint32T        maxInstances;        /**< max instances possible */
  ClUint32T        instances;           /**< current objects present */
};

typedef struct _CORMOClass  _CORMOClass_t;
typedef _CORMOClass_t*      _CORMOClass_h;

/* todo: if instances in above DS not used, remove it */

/*
 * flag
 *  participate in notification 
 *  forward
 *  persist
 */
struct CORMSOClass 
{
  ClCorClassTypeT    classId;
  ClUint16T        flags;
  ClUint16T        msoInstances; /**< MSO object instances */
};

/**
 * COR Object Meta Structure.
 * Captures the Meta structure information for MO Class.
 *
 */
struct CORObject {
  DMObjHandle_t     dmObjH;              /**< DM Object handle */
  ClUint16T        type;                /**< ClCorObjTypesT - MO / MSO */
  ClUint16T        flags;               /**< flags yet to be defined */
  ClUint8T         padding[4];
};

typedef struct CORObject    CORObject_t;
typedef CORObject_t*        CORObject_h;

/* CORObject MACROS */
#define COR_OBJECT_ISNULL(obj) ((obj).dmObjH.classId == 0)

/**
 * COR Object Pack Structure.
 */
struct CORPackObj{
    void *data;
    ClUint16T flags;
    ClUint32T size;
    ClBufferHandleT *pBufH;
};

typedef struct CORPackObj CORPackObj_t;
typedef CORPackObj_t*	 CORPackObj_h;

/**
 * COR Object Pack Structure.
 */
struct CORUnpackObj{
    void *data;
    ClUint16T instanceId;
};

typedef struct CORUnpackObj CORUnpackObj_t;
typedef CORUnpackObj_t*	 CORUnpackObj_h;

/* todo: set right what is oh, moh, tree etc., and follow
   consistency */

/* todo: 
  --> vector extended/get to be verified  (bounds check in export)
  --> efficiency interms of converting ClCorMOId to OH (to and fro)
*/

typedef MArray_t                MOTree_t;
typedef MArray_h                MOTree_h;
typedef MArrayNode_h            MOTreeNode_h;
typedef MArray_t                ObjTree_t;
typedef MArray_h                ObjTree_h;
typedef MArrayNode_h            ObjTreeNode_h;

#define COR_MO_SIZE             sizeof(_CORMOClass_t)
#define COR_MSO_SIZE            sizeof(CORMSOClass_t)
#define COR_OBJ_SIZE            sizeof(CORObject_t)

/* === The following are configuration related to 'objTree' === */

#define COR_OBJ_TAG             0xDE0000AF
/*===Don't use the following hash define use the rmdAPI to get the max buffer size supported by RMD====*/
#define COR_OBJ_BUF_SZ          0xFFFFF /* to be removed after MBUF changes */

/* === Following are macros' to operate on 'objTree' === */

#define corMOObjGet(path)       corObjTreeNodeFind(objTree,(path))
#define corMOObjParentGet(path) corObjTreeFindParent(objTree, (path))
#define corMSOObjGet(oh,svcId)  ((CORObject_h)mArrayDataGet((oh), 0, (svcId)))
#define corObjTreeShow()        corObjTreeShowDetails(0)

typedef void (*CORObjWalkFP)(void *, ClBufferHandleT);

#ifdef DEBUG
#define corObjTreeNameSet(moIdh,name) corObjTreeNodeNameSet(objTree,moIdh,name)
#define corObjTreeNodeNameSet(this,path,name) \
do  \
{ \
  ObjTreeNode_h node; \
  node = corObjTreeNodeFind(this,path);\
  if(node)\
    {\
      mArrayNodeNameSet(node, name);\
    }\
}\
while(0)
#endif

/* === The following are configuration related to 'moTree' === */

#define COR_MO_GRP_SIZE         2
#define COR_MSO_GRP_SIZE        CL_COR_SVC_ID_MAX    /* number of MSO's in one vector group */
#define COR_MO_TAG              0xAB1111AB
#define COR_MSO_TAG             0xAB2222AB
#define COR_MOTREE_BUF_SZ       8192  /* This to be removed after MBUF changes */

/* === Following are macros' to operate on 'moTree' === */

#define corMOTreeInit()         (!moTree?corMOTreeCreate(&moTree):0)
/*#define corMOTreeShow()         mArrayShow(moTree)*/
#define corMOTreeShow(cookie)         mArrayShow(moTree,cookie)
#define corMOTreeFind(path)     corMOTreeNodeFind(moTree,(path))
#define corMOTreeParentGet(path)corMOTreeFindParent(moTree, (path))
#define corMOTreeDel(path)      corMOTreeNodeDel(path)
#define corMSOGet(moh,svcId)    ((CORMSOClass_h) mArrayDataGet((moh), 0, (svcId)))
#define corMSOSet(moh,svcId,clsId) \
 ((CORMSOClass_h) mArrayExtendedDataGet((moh), 0, (svcId)))->classId = (clsId)
#define corMOTreeNodeFind(this,path) \
mArrayIdNodeFind((this),(ClUint32T *)(path)->node, (path)->depth)

#ifdef DEBUG
#define corMOTreeNameSet(path,name) corMOTreeNodeNameSet(moTree,path,name)
#define corMOTreeNodeNameSet(this,path,name) \
do  \
{ \
  MOTreeNode_h node; \
  node = corMOTreeNodeFind(this,path);\
  if(node)\
    {\
      mArrayNodeNameSet(node, name);\
    }\
}\
while(0)
#endif


/* global reference to tree root for now */
extern MOTree_h     moTree;
extern ObjTree_h    objTree;

/* prototypes go here */

ClRcT              corMOTreeCreate(MOTree_h* this);
ClRcT 	     corMoTreeFinalize(void);
ClRcT              corMOTreeAdd(MOTree_h   this, 
                                 ClCorMOClassPathPtrT  path, 
                                 ClUint32T maxInst);

ClRcT              corMOInstaceValidate(ClCorMOIdPtrT   hMOId);
    
ClRcT              corMOTreePack(ClCorMOClassPathPtrT cAddr,
                                  ClBufferHandleT *pBufH);
ClRcT              corMOTreeUnpack(ClCorMOClassPathPtrT cAddr, 
                                    void *  buf, 
                                    ClUint32T* size);
ClRcT              corMOTreeClassGet(ClCorMOClassPathPtrT path, 
                                      ClCorMOServiceIdT svcId,
                                      ClCorClassTypeT* classId);
ClRcT               _clCorMoClassPathValidate(ClCorMOClassPathT* moPath, 
                                    ClCorServiceIdT svcId);
MOTreeNode_h        corMOTreeFindParent(MOTree_h this, 
					ClCorMOClassPathPtrT path);
ClRcT				 corMOTreeNodeDel(ClCorMOClassPathPtrT path);

ClRcT 		    moPathGetNextFreeSlot(void *corPath, 
					  int *idx);
void                corMOTreeShowDetails();
ClRcT              corMOTreeUpdate();

/* object tree prototypes */
ClRcT              corObjTreeInit();
ClRcT              corObjTreeCreate(ObjTree_h* this);
ClRcT 	     corObjectTreeFinalize(void);
ClRcT              corObjTreeAdd(ObjTree_h  this, 
                                  ClCorMOIdPtrT     path,
                                  DMObjHandle_h objHandle);
ClRcT              corObjTreeNodeDelete(ObjTree_h  this, 
                                  ClCorMOIdPtrT     path);
ObjTreeNode_h       corObjTreeNodeFind(ObjTree_h this, ClCorMOIdPtrT path);
#ifdef GETNEXT
ObjTreeNode_h       corObjTreeNextNodeFind(ObjTree_h this, ClCorMOIdPtrT path);
#endif

ObjTreeNode_h       corObjTreeFindParent(ObjTree_h this, ClCorMOIdPtrT path);
ClRcT                corObjTreeShowDetails(ClCorMOIdPtrT from,
                                          ClBufferHandleT *pMsgHdl);
ClRcT              corObjTreePack(ClCorMOIdPtrT cAddr,
								   ClUint16T flags, 
                                  ClBufferHandleT *pBufH);
ClRcT              corObjTreeUnpack (ClCorMOIdPtrT moIdh, 
                                      void *  buf, 
                                      ClUint32T* size);
#ifdef REL2
ClRcT 		    moIdGetNextFreeSlot(ClCorMOIdPtrT moId, 
					int *idx);
#endif

CORObject_h     corMSOObjSet(ObjTreeNode_h oh,
                                 ClCorServiceIdT   svcId,
                                 DMObjHandle_h dmObj);

ClRcT   moId2DMHandle(ClCorMOIdPtrT path,
                                  DMObjHandle_h dmh);

Byte_h  _corMoIdToDMBufGet(ClCorMOIdT moId);

ClRcT   objHandle2DMHandle(ClCorObjectHandleT oh,
                                       DMObjHandle_h dmh);

#ifdef __cplusplus
}
#endif

#endif  /*  _INC_TREE_DEF_H_ */
