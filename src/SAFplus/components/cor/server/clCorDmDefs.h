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
 * File        : clCorDmDefs.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains Data Manager API's & Definitions
 *
 *
 *****************************************************************************/

#ifndef _INC_DATA_MGR_H_
#define _INC_DATA_MGR_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @pkg cl.cor.dataMgr */

/* INCLUDES */
#include <clCommon.h>
#include <clCorMetaData.h>
#include "clCorVector.h"
#include "clCorHash.h"
#include "clCorLlist.h"    

/******************************************************************************/

/* COR Containment MASK related routines */

#define BYTES_FOR_DM_HIERARCHY 7

typedef struct COROHContMask
{
    ClUint32T    contOHDepth;   /* number of valid enteries in the mask array */
	unsigned char contOHMask[CL_COR_OH_MAX_LEVELS]; /* object handle mask array */
} COROHContMask_t;

typedef struct COROHContLib
{
    ClUint32T  initDone;  /* library has been initialized */
    ClUint32T  OHContType;    /* Type of object handle to generate */
    COROHContMask_t OHContMask[CL_COR_OH_MAX_TYPES]; /*
	                                       * object handle masks, one each for 
										   * known handle type.
										   */
} COROHContLib_t;


extern COROHContLib_t gCorOHContLib;

/* Macros related to OH mask for contained Object */
#define gCorLocalOHContInitDone	(gCorOHContLib.initDone)
#define gCorLocalContOHType		(gCorOHContLib.OHContType)
#define gCorLocalContOHMask		(gCorOHContLib.OHContMask[gCorLocalContOHType].contOHMask)
#define gCorLocalContOHDepth (gCorOHContLib.OHContMask[gCorOHContLib.OHContType].contOHDepth)

#define gCorContOHMask(type)	(gCorOHContLib.OHContMask[type].contOHMask)
#define gCorContOHDepth(type) (gCorOHContLib.OHContMask[type].contOHDepth)


/******************************************************************************/

/* DEFINES */
/* MSB to LSB is 0-7 */
#define BYTE_SZ                    8
#define MAX_DUMP_CHARS             16

#define SET_BIT(arr,pos)   ((arr)[(pos)/BYTE_SZ]|=(1<<(BYTE_SZ-((pos)%BYTE_SZ)-1)))


/**
 * Data Manager.
 *
 * <h1>Overview</h1>
 * 
 * Data Management Routines.
 *
 * <h1>Interaction with other components</h1>
 * none
 *
 * <h1>Configuration</h1>
 *
 * <h1>Usage Scenario(s)</h1>
 *
 * @pkgdoc  cl.cor.dataMgr
 * @pkgdoctid Data Manager
 */
typedef void *      MemMgr_h;  

struct DataMgr
{
  MemMgr_h          mmgr;                         /**< Memory Manager */
  CORHashTable_h    classTable;                   /**< Class Table */
};

typedef struct DataMgr   DataMgr_t;
typedef DataMgr_t*       DataMgr_h;


/* Do not use the following Enums in application code. Use the API's
 * instead to create attributes, array attribute, association
 * attribute and containment attribute.  Also refer to the
 * documentation on how and when to use array/containment/association.
 *
 * API's are clCorClassAttributeCreate, clCorClassContainmentAttributeCreate,
 * clCorClassAssociationCreate.
 */


typedef enum CORQual {
  COR_NORMAL_ATTR,                                /**< Regular attribute */
  COR_KEY_ATTR                                    /**< Key attribute */
} CORQualifier_e;


/**
 *  COR Attribute Type.
 *  Captures COR Attribute data type.
 *     |---------------------------------------|
 *     |  ClCorTypeT (word) |  payload          |
 *     |---------------------------------------|
 *  If ClCorTypeT is PRIMITIVE
 *        payload -> CORQual
 *  If ClCorTypeT is  ARRAY
 *        payload -> ClCorTypeT
 *  If ClCorTypeT is  ASSOCIATION / CONTAINMENT
 *        payload -> associated / contained class id
 *
 */
struct CORAttrInfo {
  ClCorAttrTypeT      type;
  union 
  {
    CORQualifier_e  qualifier;                    /**< attribute qualifier */
    ClCorAttrTypeT    arrType;
    ClCorClassTypeT  remClassId;
  } u;
};

typedef struct CORAttrInfo CORAttrInfo_t;
typedef CORAttrInfo_t*     CORAttrInfo_h; 


/**
 * COR init value Type.
 *
 * Captures the min and max value range for data types.
 * Incase of association,
 *     max corresponds to max associations
 * Incase of containment, 
 *     min is min instances and 
 *     max is max instances
 * Incase of primitive data type
 *     min & max correspond to their value ranges
 * Incase of counter data type
 *     min corresponds to the init value
 *     max corresponds to the high value
 * Incase of sequence
 *    max corresponds to the delta to increment
 * Incase of array
 *    min corresponds to the first level (1d) size (can't be -1)
 *    max corresponds to the second level (2d) size (-1 means only one level)
 */
struct CORAttrValType {
  ClInt64T         init;
  ClInt64T         min;
  ClInt64T         max;
};
          
typedef struct CORAttrValType CORAttrValType_t;
typedef CORAttrValType_t*     CORAttrValType_h;


/**
 * COR Attribute Meta structure.
 * Captures the Meta structure information for COR Attribute.
 */
struct CORAttr {
  ClCorAttrIdT       attrId;                  /* key */
  CORAttrInfo_t     attrType;
  CORAttrValType_t  attrValue;
  ClInt32T         offset;                  /* relative offset within obj*/
  ClInt16T index;	/*Index of this attribute within an object */
  ClUint32T userFlags;	/* User defined flags */
};

typedef struct CORAttr CORAttr_t;
typedef CORAttr_t *    CORAttr_h;

#define COR_ATTR_SET_TYPE(a,t)          \
do {                                    \
    a.type=t;                           \
    a.u.qualifier = COR_NORMAL_ATTR;    \
} while(0)

#define COR_ATTR_IS_NORMAL(a)        ((a).type < CL_COR_MAX_TYPE) 
#define COR_ATTR_IS_CONT(a)          ((a).type == CL_COR_CONTAINMENT_ATTR) 
#define COR_ATTR_IS_ASSOC(a)         ((a).type == CL_COR_ASSOCIATION_ATTR) 
#define COR_ATTR_IS_ARRAY(a)         ((a).type == CL_COR_ARRAY_ATTR) 

#define COR_ATTR_SET_KEY(a,t)           \
do {                                    \
    a.type=t;                           \
    a.u.qualifier = COR_KEY_ATTR;       \
} while(0)

#define COR_ATTR_SET_CONT(a,rid)        \
do {                                    \
    a.type=CL_COR_CONTAINMENT_ATTR;        \
    a.u.remClassId = rid;               \
} while(0)

#define COR_ATTR_SET_VIRTUAL(a,rid)     \
do {                                    \
    a.type=CL_COR_VIRTUAL_ATTR;            \
    a.u.remClassId = rid;               \
} while(0)

#define COR_ATTR_SET_ASSOC(a,rid)       \
do {                                    \
    a.type=CL_COR_ASSOCIATION_ATTR;        \
    a.u.remClassId = rid;               \
} while(0)

#define COR_ATTR_SET_ARRAY(a,t)         \
do {                                    \
    a.type=CL_COR_ARRAY_ATTR;              \
    a.u.arrType=t;                      \
} while(0)

#define COR_CLASS_IS_BASE(hdr)        ((((hdr).flags)&(0x80)))
#define COR_CLASS_SETAS_BASE(hdr)     (hdr).flags|=(char)(0x80)
#define COR_CLASS_RESETAS_BASE(hdr)   (hdr).flags&=(char)(0x7f)

#define COR_CLASS_DEF_BLOCK_SIZE   8
#define COR_OBJ_DEF_BLOCK_SIZE    32

#define CL_COR_ATTR_WALK_RUNTIME_GET_TIMEOUT 10000

/**
 * COR Class Meta Structure.
 * Captures the Meta structure information for COR Class.
 *  flags MSB to LSB:
 *  8'th  Bit corresponds to
 *     Inherited  - means, the 'instances' field may 
 *            not be valid.
 *     Abstract  - means, the 'class is an abstract class and
 *            it may not have instances of its own
 *  Rest (6-1)
 *     Reserved
 *    
 */
struct CORClass {
  ClCorClassTypeT       classId;		          /* key */
  ClCorClassTypeT       superClassId;                 /* parent key */
  ClInt32T         size;                         
  ClVersionT         version;
  ClUint8T         flags;
  ClUint8T	moClassInstances;		/* Is any MO or MSO class is created */
  ClUint16T        nAttrs;
  CORHashTable_h    attrList;  
  CORLlist_h objFreeList;                   /* list of free indexes in DM */

  /* private attributes */
  ClUint32T        recordId;
  ClUint32T        objCount;                      /** no. of objects created */
  ClUint32T        objBlockSz;
  CORVector_t       attrs;                        /**< vector of attributes */
  CORVector_t       objects;                      /**< vector of objects */
  ClUint32T        noOfChildren;                 /**< no. of classes derived from it */
};

typedef struct CORClass CORClass_t;
typedef CORClass_t *CORClass_h;


typedef struct CORClassShowCookie
{
    CORClass_h classHdl;
    ClBufferHandleT* pMsg;
}CORClassShowCookie_t;

typedef struct CORClassWalkInfo
{
    ClHandleT            cookie;
    CORClass_h            clsHdl;
    ClCorClassAttrWalkFunc    walkFp;
}CORClassWalkInfo_t;

  /* ------------------ macro functions -------------------- */


/**
 * Check if class type given is present or not.
 * @returns
 *    CL_TRUE   - if class is present, else CL_FALSE
 */
#define dmClassIsPresent(id) (dmClassGet(id) == 0 ? CL_FALSE:CL_TRUE)

#define DM_OH_GET_TYPE(oh,t) \
do \
{\
    if(dmGlobal)\
        HASH_GET(dmGlobal->classTable, (oh).classId, t);\
}while(0)

#define DM_OH_SET(oh,t,v)               \
do {                                    \
    (oh).classId=(t)->classId;          \
    (oh).instanceId=(v);                \
} while(0)


#define DM_OH_GET_VAL(oh,v)  \
do \
{ \
  CORClass_h t=0;\
  /* is it Ok to use macro in a macro*/\
  DM_OH_GET_TYPE(*oh,t);\
  /* In case of invalid object handle (oh), class handle (t) will be null, \
  so performing this null  check. */\
  if(t != NULL) \
    v = corVectorGet(&(t->objects), oh->instanceId);\
} \
while(0)

/* Change following Macro to a function. Too complex to be a macro after
      addition of Containment Object Mask*/
#if 0
/*todo: to clean up this mess here */
#define DMohContAdd(oh,aid,idx) \
do { \
 int i; \
 for(i=0;i<4;i++) { \
   if(!(oh).u.a[i*2]) { \
     (oh).u.a[i*2]=aid; \
     (oh).u.a[i*2+1]=idx; \
     break; \
   } \
 } \
} \
while(0)

#endif

#define DM_OH_COPY(to,from)      memcpy(&(to), &(from), sizeof(from))

#define DM_OH_DBG_VERBOSE(oh,lvl,str) \
    CL_DEBUG_PRINT(lvl, (str " Object (%04x,%0x)", \
              (oh).classId, (oh).instanceId))

#define DM_OH_SHOW_VERBOSE(oh) \
 /* clOsalPrintf("(%04x,%x/%d %x-%x/%x-%x)",(oh).classId, (oh).instanceId, (oh).nextLevel, (oh).a[0], (oh).a[1], (oh).a[2],(oh).a[3]) */


/**
 * DM Object Handle.  
 *
 * Captures type and instance reference information.  On
 * serialization, conversion should happen to logical id <---> memory
 * and vice versa, so that no memory specific stuff is written.
 */
struct DMObjHandle {
  ClUint32T        classId;
  ClUint32T        instanceId;
/*
  ClUint8T	nextLevel;
  ClUint8T         a[BYTES_FOR_DM_HIERARCHY]; 
*/
};
typedef struct DMObjHandle  DMObjHandle_t;
typedef DMObjHandle_t*      DMObjHandle_h;

/**
 * DM Contained Object Handle.  
 */

struct DMContObjHandle {
  DMObjHandle_t       dmObjHandle;
  ClCorAttrPathPtrT   pAttrPath;
};
typedef struct DMContObjHandle  DMContObjHandle_t;
typedef DMContObjHandle_t*      DMContObjHandle_h;



ClRcT DMohContAdd(DMObjHandle_h oh,ClCorAttrIdT aid,ClInt32T idx);

/* global reference to datamanager for now */
extern DataMgr_h  dmGlobal;

struct _ClCorDmObjectWalkInfo {
    ClOsalCondIdT   condVar;
    ClOsalMutexIdT  condMutex;
    ClRcT           rc ;
    ClUint32T       noOfJobs;
    ClPtrT          *pAttrBuf;
    ClUint32T          *pJobStatus;
};

    /* Temporary structure for dmObjectAttrShow */
typedef struct classObjBufMsgTrio
            {
            CORClass_h         type;
            Byte_t*            buf;
            ClCorObjectHandleT objHandle;
            DMContObjHandle_h  pDmContHandle;
            ClBufferHandleT    pMsg;
            } classObjBufMsgTrioT;


typedef struct _ClCorDmObjectWalkInfo _ClCorDmObjectWalkInfoT;
typedef _ClCorDmObjectWalkInfoT * _ClCorDmObjectWalkInfoPtrT;

/* 
 * ClCorDmObjPackAttrWalker
 * (The structure is defined here, because it is being used by only clCorDmExtObjPackWithFilter
 *   and clCorDmExtObjAttrPackFun (callback). its better if kept here only.)
 * 
 * This structure is passed as cookie to dmClassAttrWalk from clCorDmExtObjPackWithFilter.
 * The cookie is finally passed to clCorDmExtObjAttrPackFun, wherein 
 * the simple attributes are packed. and contained attributes pointers are buffered, so that
 * they can be walked and packed recursively.
 */

struct ClCorDmObjPackAttrWalker{
    ClBufferHandleT  contAttrBufMsgHandle; /* stream-in the containment attribute defs here.*/
    ClUint8T                *objInstH;      /* reference to object instance (this includes CORInstanceHdr_t) */
    ClBufferHandleT  packBufMsgHandle; /* simple attributes streamed to buffer */
    ClCorObjAttrWalkFilterT   *pFilter; 
    CORClass_h              clsHdl; /*-1, means needs comparsion,(CL_TRUE or CL_FALSE)*/
    ClCorAttrPathPtrT       pCurrAttrPath;
    ClInt8T                 attrCmpResult; /*-1, means needs comparsion,(CL_TRUE or CL_FALSE)*/
    ClUint8T                srcArch; /* little endian or big endian */
    ClCorObjectHandleT      objHandle;
    ClBufferHandleT         rtAttrBufMsgHdl; /* Buffer handle to pack the attribute handle. */
};
typedef struct ClCorDmObjPackAttrWalker ClCorDmObjPackAttrWalkerT;
#ifdef __cplusplus
}
#endif

#endif  /*  _INC_DATA_MGR_H_ */
