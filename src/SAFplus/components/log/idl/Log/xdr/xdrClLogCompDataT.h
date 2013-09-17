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


/*********************************************************************
* ModuleName  : idl
*********************************************************************/
/*********************************************************************
* Description : This file contains the declarations for marshall 
*               and unmarshall funtions of ClLogCompDataT 
*     
* THIS FILE IS AUTO-GENERATED BY OPENCLOVIS IDE. EDIT THIS FILE AT
* YOUR OWN RISK. ANY CHANGE TO THIS FILE WILL BE OVERWRITTEN ON
* RE-GENERATION.
*     
*********************************************************************/

#ifndef _XDR_CL_LOG_COMP_DATA_T_H_
#define _XDR_CL_LOG_COMP_DATA_T_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "clXdrApi.h"

#include "clIocApi.h"
#include "clLogApi.h"

struct _ClLogCompDataT_4_0_0;

typedef struct _ClLogCompDataT_4_0_0 {
    SaNameT    compName;
    ClUint32T    clientId;

}ClLogCompDataT_4_0_0;


ClRcT  clXdrMarshallClLogCompDataT_4_0_0(void *,ClBufferHandleT , ClUint32T);

ClRcT  clXdrUnmarshallClLogCompDataT_4_0_0(ClBufferHandleT, void *);

#define clXdrMarshallArrayClLogCompDataT_4_0_0(pointer, multiplicity, msg, isDelete) \
clXdrMarshallArray((pointer), sizeof(ClLogCompDataT_4_0_0), (multiplicity), clXdrMarshallClLogCompDataT_4_0_0, (msg), (isDelete))

#define clXdrUnmarshallArrayClLogCompDataT_4_0_0(msg,pointer, multiplicity) \
clXdrUnmarshallArray((msg),(pointer), sizeof(ClLogCompDataT_4_0_0), (multiplicity), clXdrUnmarshallClLogCompDataT_4_0_0)

#define clXdrMarshallPointerClLogCompDataT_4_0_0(pointer, multiplicity, msg, isDelete) \
clXdrMarshallPointer((pointer), sizeof(ClLogCompDataT_4_0_0), (multiplicity), clXdrMarshallClLogCompDataT_4_0_0, (msg), (isDelete))

#define clXdrUnmarshallPointerClLogCompDataT_4_0_0(msg,pointer) \
clXdrUnmarshallPointer((msg),(pointer), sizeof(ClLogCompDataT_4_0_0), clXdrUnmarshallClLogCompDataT_4_0_0)

#define clXdrMarshallPtrClLogCompDataT_4_0_0(pointer, multiplicity, msg, isDelete) \
clXdrMarshallPtr((pointer), sizeof(ClLogCompDataT_4_0_0), (multiplicity), clXdrMarshallClLogCompDataT_4_0_0, (msg), (isDelete))

#define clXdrUnmarshallPtrClLogCompDataT_4_0_0(msg,pointer,multiplicity) \
clXdrUnmarshallPtr((msg),(pointer), sizeof(ClLogCompDataT_4_0_0),multiplicity, clXdrUnmarshallClLogCompDataT_4_0_0)


typedef ClLogCompDataT_4_0_0 ClLogCompDataT;



#ifdef __cplusplus
}
#endif

#endif /*_XDR_CL_LOG_COMP_DATA_T_H_*/
