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
 * File        : clCorUtilsProtoType.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


#ifndef _utils_ProtoType_h_
#define _utils_ProtoType_h_

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES */
#include <clCommon.h>
#include <clCorMetaData.h>
#include "clCorTreeDefs.h"
#include "clCorDefs.h"

/* DEFINES */

extern ClRcT corUsrBufPtrByMOIdGet(ClCorMOIdPtrT moId, void **uBuf);
extern ClRcT corObjUsrBufPtrGet(ClCorMOIdPtrT cAddr, void **uBuf);
extern ClRcT corMOId2CardIdGet(ClCorMOIdPtrT moId, ClUint16T *cardId);
extern ClRcT corMOIdExprCreate(ClCorMOIdPtrT moId, ClRuleExprT* *pRbeExpr);

/* following api's from demo are being referred, needs to be removed */

extern ClRcT corGetNameByKey( corContKey_t *key, char **name );
extern ClRcT corXlatePath(char *path, ClCorMOClassPathPtrT cAddr);
extern void corMOIdPrint(ClCorMOIdPtrT cAddr);
extern ClRcT corCardID2MOIdGet(ClUint16T cardId, ClCorMOIdPtrT moId);

#ifdef __cplusplus
}
#endif

#endif  /* _dm_ProtoType_h_ */
