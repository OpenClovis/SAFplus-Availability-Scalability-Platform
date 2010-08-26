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
 * File        : clCorEoFunc.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *  This file contains the prototypes of COR's Service interface.
 *
 *
 *****************************************************************************/

#ifndef _CL_COR_EO_FUNC_H_
#define _CL_COR_EO_FUNC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clEoApi.h>

/*****************************************************************************
 *  Functions
 *****************************************************************************/
ClRcT  VDECL(_corSyncRequestProcess) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_corEOEvtHandler) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_corEOSyncStatusGet) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_corEOUpdMoidInitiate) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_corNIOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_corClassOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_corAttrOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_corMOTreeClassOpRmd) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_corRouteGet) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_corPersisOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_corObjectOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_corObjectHandleConvertOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_corObjectWalkOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_corRouteApi) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_corObjectFlagsOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(clCorTransactionConvertOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_corStationDisable) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_CORNotifyGetAttrBits) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_CORUtilOps) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_CORUtilExtendedOps) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                                  ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_corMoIdToNodeNameTableOp) (ClEoDataT cData, ClBufferHandleT inMsgHandle, 
									ClBufferHandleT outMsgHandle);

ClRcT VDECL(_clCorDelaysRequestProcess) (ClEoDataT eoData, ClBufferHandleT inMsgH, 
                                    ClBufferHandleT outMsgH);
#ifdef GETNEXT
ClRcT  VDECL(_corObjectGetNextOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);
#endif

ClRcT VDECL(_corObjLocalHandleGet) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_corMOIdOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_corObjExtObjectPack) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                              ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_clCorBundleOp) (ClEoDataT data, ClBufferHandleT  inMsgHandle,
                                     ClBufferHandleT  outMsgHandle);

ClRcT VDECL(_clCorClientDbgCliExport) ( ClEoDataT eoData, ClBufferHandleT inMsgHandle, 
                                    ClBufferHandleT outMsgHandle );
ClRcT VDECL(_clCorMsoConfigOp) ( ClEoDataT eoData, ClBufferHandleT inMsgHandle, 
                                    ClBufferHandleT outMsgHandle );
#ifdef __cplusplus
}
#endif

#endif /* _CL_COR_EO_FUNC_H_ */
