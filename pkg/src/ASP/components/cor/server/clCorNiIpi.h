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
 * File        : clCorNiIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains ipi declarations of NI table
 *
 *
 *****************************************************************************/

#ifndef _CL_COR_NI_IPI_H_
#define _CL_COR_NI_IPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCorMetaData.h>
#include <clBufferApi.h>

/* NI Table */
ClRcT corNiInit();
void  corNiTableShow(ClBufferHandleT *pMsgHdl);
ClUint32T  corNiTablePack(void *  *pBuf, ClUint32T   *pLen);
ClUint32T  corNiTableUnPack(void * pBuf, ClUint32T  len);


ClRcT    _corNIClassDel(ClCorClassTypeT classId);
ClRcT    _corNiNameToKeyGet(char *name, ClCorClassTypeT *pKey);
ClRcT    _corNiKeyToNameGet(ClCorClassTypeT key, char *name );
ClRcT    _corNIClassAdd(char *name, ClCorClassTypeT key);
ClRcT    _corNIAttrAdd(ClCorClassTypeT  classId, ClCorAttrIdT  attrId, char *name);
ClRcT    _corNIAttrDel(ClCorClassTypeT  classId, ClCorAttrIdT  attrId);
ClRcT    _corNIAttrNameGet(ClCorClassTypeT  classId, ClCorAttrIdT  attrId, char *name);
ClRcT    _corNIAttrKeyGet(ClCorClassTypeT  classId, char *name,  ClCorAttrIdT  *pAttrId);

#ifdef __cplusplus
}
#endif

#endif  /* _CL_COR_API_H_ */
