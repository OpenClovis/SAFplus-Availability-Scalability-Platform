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
 * File        : clCorRt.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains Route Manager API's & Definitions
 *
 *
 *****************************************************************************/

#ifndef _INC_ROUTE_H_
#define _INC_ROUTE_H_
#ifdef __cplusplus
extern "C" {
#endif
                                                                                                 
/** @pkg cl.cor.rm */
                                                                                                 
/* INCLUDES */
#include <clCommon.h>
#include <clRbTree.h>
#include <clCorMetaData.h>
#include "clCorVector.h"

/*
 * Flags
 *
 *  MSB->LSB
 *   16'th bit -- 1 -> (No notification)
 *   15'th bit -- 1 -> (No persistence)
 *   14'th bit -- 1 -> (forward node)
 *   13'th bit -- 1 -> (physical node,that means, no caching/not in sync list)
 *
 */
struct RouteInfo
{
    ClCorMOIdPtrT            moH;
    ClRuleExprT*         exprH;
    struct CORVector  stnVector;
    ClUint16T        nStations;
    ClUint16T        nPrimaryOI;
    ClUint16T flags;
    ClVersionT        version;
    ClUint32T index;
    ClRbTreeT tree;
};
typedef struct RouteInfo RouteInfo_t;
typedef RouteInfo_t*     RouteInfo_h;

/* temporary RouteConfig Pack structure */
struct RouteInfoStream
{
    void *      data;
    Byte_h      buf;
    ClUint32T  size;
    ClBufferHandleT *pBufH;
};
typedef struct RouteInfoStream   RouteInfoStream_t;
typedef RouteInfoStream_t*       RouteInfoStream_h;

ClRcT  corObjAttrFind(ClCorObjectHandleT this, ClCorAttrPathPtrT pAttrPath,
                       ClCorAttrIdT attrId, ClInt32T index, 
                       void * pValue, ClUint32T *pSize);
/**
  * Function to check whether the OI being added as the primary OI is already there in the 
  * the list when the primary. If yes then return success but if not then return error.
  */ 
ClRcT
_clCorPrimaryOIEntryCheck( RouteInfo_t *pRt, ClCorAddrT *pCompAddr, ClCorMOIdPtrT pMoId );

#ifdef __cplusplus
}
#endif
#endif  /*  _INC_RT_H*/
