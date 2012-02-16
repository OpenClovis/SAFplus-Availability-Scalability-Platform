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
 * File        : clCorObj.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


#ifndef _COR_OBJ_ProtoType_h_
#define _COR_OBJ_ProtoType_h_

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES */
#include <clCorMetaData.h>
#include <clCorNotifyApi.h>

/* Internal Include */
#include <clCorNotifyCommon.h>


	
/* DEFINES */

extern ClRcT _corMSOObjDelete(ClCorMOIdPtrT moId);
extern ClRcT _corMOObjCreate(ClCorMOIdPtrT moId);
extern ClRcT _corMSOObjCreate(ClCorMOIdPtrT this, ClUint32T srvcId);
extern ClRcT _corMOObjDelete(ClCorMOIdPtrT moId);
extern ClRcT corCompInit(ClUint32T numTxns, ClUint32T eoPeriodicId, ClUint32T remCardID);

extern ClRcT corObjHandleLock(ClCorObjectHandleT objHandle);
extern ClRcT corObjHandleUnlock(ClCorObjectHandleT objHandle);

extern ClRcT corObjAttrBitsGet(ClCorMOIdT  *moId, ClCorAttrPathPtrT pAttrPath, ClCorAttrListPtrT attList, ClCorAttrBitsT *pAttrBits);

#if 0
extern ClRcT corObjCopyGet(ClCorMOIdPtrT moIdh, ClCorServiceIdT svc, !!!OBSOLETEvPtr_t!!! pBuf, ClUint32T *pSize);
extern ClRcT _corObjCreate(ClCorMOIdPtrT moId, ClCorServiceIdT srvcId, ClCorObjectHandleT *handle);
extern ClRcT _corObjDelete(ClCorObjectHandleT handle);
extern ClRcT corMOObjCreate(ClCorTxnIdPtrT tid, ClCorMOIdPtrT moId, ClCorObjectHandleT *handle);
extern ClRcT corMSOObjCreate(ClCorTxnIdPtrT tid, ClCorMOIdPtrT moId, ClUint32T srvcId, ClCorObjectHandleT *objRef);
#endif
extern ClRcT corMOObjHandleGet(ClCorMOIdPtrT this, ClCorObjectHandleT *objHandle);
#if 0
extern ClRcT corMOObjDelete(ClCorTxnIdPtrT tid, ClCorObjectHandleT handle);
extern ClRcT corMSOObjDelete(ClCorTxnIdPtrT tid, ClCorObjectHandleT handle);
#endif
extern ClRcT corMSOObjHandleGet(ClCorMOIdPtrT this, ClUint32T srvcId, ClCorObjectHandleT *objRef);
extern ClRcT _clCorObjectValidate(ClCorMOIdPtrT pMoId);

#ifdef __cplusplus
}
#endif

#endif  /* _COR_OBJ_ProtoType_h_ */
