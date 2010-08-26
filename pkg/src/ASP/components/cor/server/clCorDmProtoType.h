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
 * File        : clCorDmProtoType.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains the definitions for the Base Object. Also, this  
 *  file contains the function prototypes of the interfaces that are   
 *  exported.    
 *
 *
 *****************************************************************************/

#ifndef _dm_ProtoType_h_
#define _dm_ProtoType_h_

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES */
#include <clCommon.h>
#include <clCorMetaData.h>
#include "clCorClient.h"
#include "clCorDmDefs.h"
#include "clCorUtilsProtoType.h"
#include "clBufferApi.h"

/* DEFINES */

/* FORWARD DECLARATION */

/* clCorDmAttr.c */
ClRcT dmClassAttrCompare(CORAttr_h this, CORAttr_h cmp);
ClRcT dmClassAttrPack(CORAttr_h this, ClBufferHandleT *pBuf);
ClInt32T dmClassAttrUnpack(CORAttr_h this, void * src);
ClRcT dmClassAttrInit(CORAttr_h this, ClInt64T init, ClInt64T min, ClInt64T max);
ClRcT dmClassCompare(CORClass_h this, CORClass_h cmp);
ClInt32T dmClassAttrTypeSize(CORAttrInfo_t at, CORAttrValType_t val);
ClRcT dmObjectAttrValidate(CORAttr_h this, Byte_h attrBuf);
ClRcT dmClassAttrPos(CORAttr_h this, Byte_h*pos);
#if 0
ClRcT dmClassAttrType2Id(CORAttr_h this, Byte_h*pos);
#endif
#ifdef REL2
ClRcT dmObjectType2Id(DMObjHandle_h this, ClCorClassTypeT atype, ClCorAttrIdT*   aid);
#endif
ClRcT dmClassAttrSize(CORAttr_h this, Byte_h*len);
ClRcT dmObjectAttrInit(CORAttr_h this, Byte_h*buf);
ClRcT dmObjectAttrBufCopy(CORAttr_h this, Byte_h instBuf, Byte_h attrBuf, ClUint32T *size, ClInt32T idx, int flag);
/* clCorDmClass.c */
ClRcT dmClassAttrAdd(CORClass_h this, ClCorAttrIdT id, CORAttrInfo_t typ, CORAttr_h *attr);
ClRcT dmClassAttrWalk(CORClass_h this, CORAttr_h till, ClRcT (*attrfp)(CORAttr_h, Byte_h*), Byte_h* buf);
CORAttr_h dmClassAttrGet(CORClass_h this, ClCorAttrIdT aid);
ClRcT dmObjectAttrGet(DMContObjHandle_h this, ClCorAttrIdT aid, ClInt32T idx, void * value, ClUint32T *size);
ClRcT dmObjectContGet(DMObjHandle_h this, ClCorAttrIdT aid, DMObjHandle_h containedObject, ClInt32T idx);
ClRcT dmObjectAttrSet(DMContObjHandle_h this, ClCorAttrIdT aid, ClInt32T idx, void * value, ClUint32T size);
ClRcT dmObjectBufferAttrSet(CORClass_h classH,
                                void* instBuf,
                                ClCorAttrIdT attrId,
                                ClInt32T index,
                                void* value,
                                ClUint32T size);
ClRcT dmClassAttrDel(CORClass_h this, ClCorAttrIdT aid);
ClRcT dmClassParentSet(CORClass_h this, ClUint16T parentId);
ClUint16T dmClassParentGet(CORClass_h this);
ClRcT dmClassVersionSet(CORClass_h this, ClUint8T ver);
ClUint8T dmClassVersionGet(CORClass_h this);
ClRcT dmClassPersistOn(CORClass_h this);
ClRcT dmClassPersistOff(CORClass_h this);
ClUint8T dmClassPersistGet(CORClass_h this);
ClRcT dmClassForwardOn(CORClass_h this);
ClRcT dmClassForwardOff(CORClass_h this);
ClUint8T dmClassForwardGet(CORClass_h this);
ClRcT dmClassPack(CORHashKey_h key, CORHashValue_h classBuf, void * userArg, ClUint32T dataLength);
ClUint32T dmClassUnpack(void * buf, CORClass_h* ch);
#if 0
ClUint32T dmClassInstanceCountGet(ClCorClassTypeT classId);
#endif
ClRcT dmClassBitmap(ClCorClassTypeT classId, ClCorAttrIdT *attrList, ClInt32T nAttrs, ClUint8T *bits, ClUint32T bitMaskSz);
  
/* dmCompId.c */
/* clCorDmMain.c */
DataMgr_h dmCreate(MemMgr_h mem);
ClRcT dmDelete(DataMgr_h this);
ClRcT dmInit(void);
void dmFinalize(void);
ClRcT dmClassCreate(ClCorClassTypeT id, ClCorClassTypeT inhId);
ClRcT dmClassByHandleDelete(CORClass_h classHandle);
ClRcT dmClassDelete(ClCorClassTypeT id);
CORClass_h dmClassGet(ClCorClassTypeT id);
ClRcT dmObjectInit(DMObjHandle_h objHandle);
ClRcT dmObjectInitializedAttrSet(DMObjHandle_h objHandle, ClCorInstanceIdT instId);
ClRcT dmObjectCreate(ClCorClassTypeT id, DMObjHandle_h *objHandle);
ClRcT dmObjectDelete(DMObjHandle_h objHandle);
ClRcT dmObjectLock(DMObjHandle_h objHandle);
ClRcT dmObjectUnlock(DMObjHandle_h objHandle);
ClRcT dmObjectEnable(DMObjHandle_h objHandle);
ClRcT dmObjectDisable(DMObjHandle_h objHandle);

ClRcT  dmObjectPack(CORHashKey_h key, CORHashValue_h buffer, ClBufferHandleT *pBufH);
ClRcT  dmObjectUnpack(void * contents, DMObjHandle_h* oh, ClInt16T instanceId); 

ClRcT  dmObjHandlePack( ClBufferHandleT *pBufH, DMObjHandle_h oh, ClUint32T *size);
ClUint32T dmObjHandleUnpack(void * buf, DMObjHandle_h* oh, ClInt16T instanceId); 

ClRcT dmClassShow(CORHashKey_h key, CORHashValue_h classBuf, void * userArg, ClUint32T dataLength);
ClRcT dmClassSize(CORHashKey_h key, CORHashValue_h classBuf, void * userArg, ClUint32T dataLength);
ClRcT dmClassVerboseShow(CORHashKey_h key, CORHashValue_h classBuf, void * userArg, ClUint32T dataLength);

/*void dmShow(char**,ClBufferHandleT *);*/
void dmShow(char*,ClBufferHandleT *);
ClRcT dmObjectAttrShow(CORAttr_h this, Byte_h* buf);
ClUint32T dmClassesPack(void *buf);
ClRcT dmClassesUnpack(void *, ClUint32T);
ClRcT dmObjectCopy(DMObjHandle_h objHandle, ClBufferHandleT *pBufH, ClUint32T *size);
ClRcT dmObjectBitmap(DMContObjHandle_h objHandle, ClCorAttrIdT *attrList, ClInt32T nAttrs, ClUint8T *bits, ClUint32T bitMaskSz);
/* clCordmObject.c */
char*  dmClassNameGet(CORClass_h this);
ClRcT dmClassNameSet(CORClass_h this, clInt8_h name );
char*  dmClassAttrNameGet(CORAttr_h this);
ClRcT dmClassAttrNameSet(CORAttr_h this, clInt8_h name);
CORAttr_h dmObjectAttrTypeGet(DMContObjHandle_h this, ClCorAttrIdT    aid);

ClRcT dmClassAttrCreate(CORClass_h   classHdl, corAttrIntf_t  *pAttr);
ClRcT dmObjectAttrHandleGet(DMContObjHandle_h dmHandle, CORClass_h *pClass, void * *pInstance);
void  dmBinaryShow(Byte_h title, Byte_h buf, ClUint32T len);
ClRcT dmAttrPathToClassIdGet(ClCorClassTypeT containerClsId,
                                    ClCorAttrPathPtrT pAttrPath, ClCorClassTypeT *containedClsId);

ClRcT dmClassAttrShow(ClUint32T idx, void * element, void ** cookie);
ClRcT dmObjectShow(DMContObjHandle_h dmContObjHdl, ClBufferHandleT msg, ClCorObjectHandleT objHandle);
#ifdef REL2
ClRcT  dmObjHandleShow(DMObjHandle_h objHandle);
#endif
ClRcT dmObjectAttrIndexSet(CORAttr_h this, Byte_h*   buf);
ClRcT dmObjectAttrIndexGet(DMContObjHandle_h contObjHandle, ClCorAttrIdT attrId, ClUint16T* Idx);
ClRcT dmObjectAttrIdGet(CORAttr_h this, Byte_h*   buf);
ClRcT dmObjectAttrIndexToId(DMObjHandle_h objHandle, ClUint16T Idx, ClCorAttrIdT* attrId);
ClRcT clCorDmAttrValCompare(CORAttr_h attrH, ClCorDmObjPackAttrWalkerT *pBuf, ClCorAttrIdT attrId, ClInt32T index,   
                             void   * value, ClUint32T  size, ClCorAttrCmpFlagT cmpFlag);
ClRcT
clCorDmExtObjPack(DMObjHandle_h objHandle, ClUint8T srcArch, ClCorObjAttrWalkFilterT *pWalkFilter, 
        ClBufferHandleT bufMsgHandle, ClCorObjectHandleT objHdl);

ClRcT   _clCorDmAttrRtAttrGet(ClCorDmObjPackAttrWalkerT *pBuf, CORAttr_h attrH, 
                      ClPtrT *pAttrBuf);
ClRcT dmClassAttrListGet(ClCntKeyHandleT key,
                    ClCntDataHandleT data,
                    void* userArg,
                    ClUint32T userArgSize);
#ifdef COR_TEST
  void dmTestCreateSchema();
  void dmTestMain();
  void corContainmentTest();
#endif

#ifdef __cplusplus
}
#endif

#endif  /* _dm_ProtoType_h_ */
